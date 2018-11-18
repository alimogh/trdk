//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystem.hpp"
#include "Request.hpp"
#include "Session.hpp"
#include "TradingSystemConnection.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Binance;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace b = Binance;
namespace net = Poco::Net;

namespace {

class BinanceOrderTransactionContext : public OrderTransactionContext {
 public:
  explicit BinanceOrderTransactionContext(trdk::TradingSystem &tradingSystem,
                                          OrderId orderId,
                                          ProductId productId)

      : OrderTransactionContext(tradingSystem, std::move(orderId)),
        m_productId(std::move(productId)) {}

  const ProductId &GetProductId() const { return m_productId; }

 private:
  ProductId m_productId;
};

}  // namespace

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &config) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto &result = boost::make_shared<b::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), config);
  return result;
}

b::TradingSystem::TradingSystem(const App &,
                                const TradingMode &mode,
                                Context &context,
                                std::string instanceName,
                                std::string title,
                                const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_session(CreateSession(m_settings, true)) {}

bool b::TradingSystem::IsConnected() const { return !m_key.empty(); }

Volume b::TradingSystem::CalcCommission(const Qty &qty,
                                        const Price &price,
                                        const OrderSide &,
                                        const trdk::Security &) const {
  return (qty * price) * (0.1 / 100);
}

void b::TradingSystem::CreateConnection() {
  GetLog().Debug("Creating connection...");

  Assert(m_key.empty());
  Assert(!m_products);
  try {
    GetLog().Debug(
        "Server time: %1%.",
        ConvertToPTimeFromMicroseconds(
            boost::get<1>(PublicRequest("v1/time", "", GetContext(), GetLog())
                              .Send(m_session))
                .get<intmax_t>("serverTime") *
            1000) -
            m_serverTimeDiff);

    const auto &products = GetProductList(m_session, GetContext(), GetLog());

    const auto keyRequestResponse =
        PrivateRequest("v1/userDataStream", net::HTTPRequest::HTTP_POST,
                       GetContext(), m_settings, GetLog(), GetTradingLog())
            .Send(m_session);

    auto key = boost::get<1>(keyRequestResponse).get<std::string>("listenKey");
    if (key.empty()) {
      throw ConnectError("Failed to request listen key (result is empty)");
    }

    const auto &balanceRequestResponse =
        SignedRequest("v3/account", net::HTTPRequest::HTTP_GET, "",
                      GetContext(), m_settings, GetLog(), GetTradingLog())
            .Send(m_session);
    for (const auto &symbol :
         boost::get<1>(balanceRequestResponse).get_child("balances")) {
      m_balances.Set(symbol.second.get<std::string>("asset"),
                     symbol.second.get<Volume>("free"),
                     symbol.second.get<Volume>("locked"));
    }

    const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
    auto listeningConnection = CreateListeningConnection();

    m_products = &products;
    m_key = std::move(key);
    m_listeningConnection = std::move(listeningConnection);

  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

std::unique_ptr<OrderTransactionContext> b::TradingSystem::SendOrderTransaction(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side,
    const TimeInForce &tif) {
  static_assert(numberOfTimeInForces == 5, "List changed.");
  switch (tif) {
    case TIME_IN_FORCE_IOC:
      return SendOrderTransactionAndEmulateIoc(security, currency, qty, price,
                                               params, side);
    case TIME_IN_FORCE_GTC:
      break;
    default:
      throw Exception("Order time-in-force type is not supported");
  }
  if (currency != security.GetSymbol().GetCurrency()) {
    throw Exception("Trading system supports only security quote currency");
  }
  if (!price) {
    throw Exception("Market order is not supported");
  }

  const auto &symbol = security.GetSymbol().GetSymbol();
  const auto &productIt = m_products->find(symbol);
  if (productIt == m_products->cend()) {
    throw Exception("Product is unknown");
  }
  const auto &product = productIt->second;

  boost::format requestParams(
      "symbol=%1%&side=%2%&type=LIMIT&timeInForce=GTC&quantity=%3%&price=%4%");
  requestParams % product.id                       // 1
      % (side == ORDER_SIDE_BUY ? "BUY" : "SELL")  // 2
      % (product.qtyFilter
             ? RoundByPrecisionPower(qty, product.qtyFilter->precisionPower)
             : qty)  // 3
      % (product.priceFilter ? RoundByPrecisionPower(
                                   *price, product.priceFilter->precisionPower)
                             : *price);  // 4

  SignedRequest request("v3/order", net::HTTPRequest::HTTP_POST,
                        requestParams.str(), GetContext(), m_settings, GetLog(),
                        GetTradingLog());
  const auto response = boost::get<1>(request.Send(m_session));

  try {
    return boost::make_unique<BinanceOrderTransactionContext>(
        *this, response.get<std::string>("orderId"), product.id);
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        R"(Wrong server response to the request "%1%" (%2%): "%3%")");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void b::TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  const auto &binanceOrderTransaction =
      *boost::polymorphic_cast<const BinanceOrderTransactionContext *>(
          &transaction);
  SignedRequest(
      "v3/order", net::HTTPRequest::HTTP_DELETE,
      "symbol=" + binanceOrderTransaction.GetProductId() +
          "&orderId=" + binanceOrderTransaction.GetOrderId().GetValue(),
      GetContext(), m_settings, GetLog(), GetTradingLog())
      .Send(m_session);
}

boost::shared_ptr<TradingSystemConnection>
b::TradingSystem::CreateListeningConnection() {
  auto result = boost::make_shared<TradingSystemConnection>();
  result->Connect();
  result->Start(
      m_key,
      TradingSystemConnection::Events{
          []() -> const TradingSystemConnection::EventInfo { return {}; },
          [this](const TradingSystemConnection::EventInfo &,
                 const ptr::ptree &message) { HandleMessage(message); },
          [this]() {
            const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
            if (!m_listeningConnection) {
              GetLog().Debug("Disconnected.");
              return;
            }
            const auto connection = std::move(m_listeningConnection);
            GetLog().Warn("Connection lost.");
            GetContext().GetTimer().Schedule(
                [this, connection]() {
                  ScheduleListeningConnectionReconnect();
                },
                m_timerScope);
          },
          [this](const std::string &event) { GetLog().Debug(event.c_str()); },
          [this](const std::string &event) { GetLog().Info(event.c_str()); },
          [this](const std::string &event) { GetLog().Warn(event.c_str()); },
          [this](const std::string &event) { GetLog().Error(event.c_str()); }});
  return result;
}

void b::TradingSystem::ScheduleListeningConnectionReconnect() {
  GetContext().GetTimer().Schedule(
      [this]() {
        const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
        GetLog().Info("Reconnecting...");
        Assert(!m_listeningConnection);
        try {
          m_listeningConnection = CreateListeningConnection();
        } catch (const std::exception &ex) {
          GetLog().Error("Failed to connect: \"%1%\".", ex.what());
          ScheduleListeningConnectionReconnect();
          return;
        }
      },
      m_timerScope);
}

void b::TradingSystem::HandleMessage(const ptr::ptree &message) {
  const auto &type = message.get<std::string>("e");
  if (type == "outboundAccountInfo") {
    UpdateBalances(message);
  } else if (type == "executionReport") {
    UpdateOrder(message);
  }
}

void b::TradingSystem::UpdateBalances(const ptr::ptree &message) {
  for (const auto &symbol : message.get_child("B")) {
    m_balances.Set(symbol.second.get<std::string>("a"),
                   symbol.second.get<Volume>("f"),
                   symbol.second.get<Volume>("l"));
  }
}

void b::TradingSystem::UpdateOrder(const ptr::ptree &message) {
  GetTradingLog().Write("response-dump executionReport\t%1%",
                        [&message](TradingRecord &record) {
                          record % Lib::ConvertToString(message, false);  // 1
                        });

  const auto &id = message.get<OrderId>("i");
  const auto &time =
      ConvertToPTimeFromMicroseconds(message.get<int64_t>("E") * 1000) -
      m_serverTimeDiff;
  const auto &status = message.get<std::string>("X");
  if (status == "NEW") {
    OnOrderOpened(time, id);
    return;
  }
  if (status == "TRADE") {
    Trade trade{message.get<Price>("l"), message.get<Qty>("L"),
                message.get<std::string>("t")};
    OnTrade(time, id, std::move(trade));
  }

  const auto &qty = message.get<Qty>("q");
  const auto &filledQty = message.get<Qty>("z");
  AssertLe(filledQty, qty);
  const auto remainingQty = qty - filledQty;
  const auto &commission = message.get<Volume>("n");

  if (status == "CANCELED") {
    OnOrderCanceled(time, id, remainingQty, commission);
  } else if (status == "REJECTED") {
    OnOrderError(time, id, remainingQty, commission,
                 message.get<std::string>("r"));
  } else if (status == "FILLED") {
    OnOrderFilled(time, id, commission);
  } else {
    boost::format error("Unknown or unsupported order status \"%1%\"");
    error % status;  // 1
    OnOrderError(time, id, remainingQty, commission, error.str());
  }
}

Balances &b::TradingSystem::GetBalancesStorage() { return m_balances; }

boost::optional<OrderCheckError> b::TradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  auto result = Base::CheckOrder(security, currency, qty, price, side);

  const auto &productIt = m_products->find(security.GetSymbol().GetSymbol());
  if (productIt == m_products->cend()) {
    GetLog().Error(R"(Failed find product for "%1%" to check order.)",
                   security);
    throw Exception("Symbol is not supported by exchange");
  }
  const auto &product = productIt->second;

  if (price) {
    if (product.priceFilter) {
      if (!result || !result->price) {
        if (*price < product.priceFilter->min) {
          if (!result) {
            result = OrderCheckError{};
          }
          result->price = product.priceFilter->min;
        } else if (*price > product.priceFilter->max) {
          if (!result) {
            result = OrderCheckError{};
          }
          result->price = product.priceFilter->max;
        }
      }
    }

    if (product.minVolume && (!result || !result->volume) &&
        (qty * *price) < *product.minVolume) {
      if (!result) {
        result = OrderCheckError{};
      }
      result->volume = product.minVolume;
    }
  }

  if (product.qtyFilter && (!result || !result->qty)) {
    if (qty < product.qtyFilter->min) {
      if (!result) {
        result = OrderCheckError{};
      }
      result->qty = product.qtyFilter->min;
    } else if (qty > product.qtyFilter->max) {
      if (!result) {
        result = OrderCheckError{};
      }
      result->qty = product.qtyFilter->max;
    }
  }

  return result;
}

bool b::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products->count(symbol) > 0;
}
