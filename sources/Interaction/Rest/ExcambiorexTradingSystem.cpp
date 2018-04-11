/*******************************************************************************
 *   Created: 2018/02/11 21:53:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ExcambiorexTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

ExcambiorexTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                             ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

ExcambiorexTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    const std::string &params,
    const Settings &settings,
    bool isPriority,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : Base(name, net::HTTPRequest::HTTP_POST, params, context, log, tradingLog),
      m_settings(settings),
      m_isPriority(isPriority) {}

void ExcambiorexTradingSystem::PrivateRequest::CreateBody(
    const net::HTTPClientSession &session, std::string &result) const {
  {
    using namespace trdk::Lib::Crypto;
    const auto nonce = static_cast<uint32_t>(
        pt::microsec_clock::universal_time().time_of_day().total_nanoseconds());
    const auto &digest = Hmac::CalcSha512Digest(
        boost::lexical_cast<std::string>(nonce) + "-" + m_settings.apiKey,
        m_settings.apiSecret);
    boost::format auth("key=%1%&signature=%2%&nonce=%3%");
    auth % m_settings.apiKey                                            // 1
        % boost::to_upper_copy(EncodeToHex(&digest[0], digest.size()))  // 2
        % nonce;                                                        // 3
    AppendUriParams(auth.str(), result);
  }

  Base::CreateBody(session, result);
}

bool ExcambiorexTradingSystem::PrivateRequest::IsPriority() const {
  return m_isPriority;
}

ExcambiorexTradingSystem::BalancesRequest::BalancesRequest(
    const Settings &settings, const Context &context, ModuleEventsLog &log)
    : PrivateRequest("GetInfo", "", settings, false, context, log) {}

ExcambiorexTradingSystem::OrderRequest::OrderRequest(
    const std::string &name,
    const std::string &params,
    const Settings &settings,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog &tradingLog)
    : PrivateRequest(name, params, settings, true, context, log, &tradingLog) {}

ExcambiorexTradingSystem::ActiveOrdersRequest::ActiveOrdersRequest(
    const std::string &params,
    const Settings &settings,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog &tradinLog)
    : PrivateRequest(
          "ActiveOrders", params, settings, false, context, log, &tradinLog) {}

////////////////////////////////////////////////////////////////////////////////

ExcambiorexTradingSystem::ExcambiorexTradingSystem(
    const App &,
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings, GetContext(), GetLog()),
      m_tradingSession(CreateExcambiorexSession(m_settings, true)),
      m_pollingSession(CreateExcambiorexSession(m_settings, false)),
      m_orderRequestListVersion(0),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

bool ExcambiorexTradingSystem::IsConnected() const {
  return !m_products.empty();
}

void ExcambiorexTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    std::tie(m_products, m_currencies) =
        RequestExcambiorexProductAndCurrencyList(m_tradingSession, GetContext(),
                                                 GetLog());
    UpdateBalances();
#if 0
    ActiveOrdersRequest("", m_settings, GetContext(), GetLog(), GetTradingLog())
        .Send(m_tradingSession);
#endif
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  for (auto &product : m_products) {
    Assert(!product.oppositeProduct);
    for (const auto &oppositeProduct : m_products) {
      if (oppositeProduct.directId == product.oppositeId) {
        if (product.oppositeProduct) {
          GetLog().Error(
              "Duplicate opposite product \"%1%\" object found for product "
              "\"%2%\" that already has opposite product object.",
              oppositeProduct.directId,  // 1
              product.directId);         // 2
          throw ConnectError("Failed to build product list");
        }
        const_cast<const ExcambiorexProduct *&>(product.oppositeProduct) =
            &oppositeProduct;
      }
    }
    if (!product.oppositeProduct) {
      GetLog().Error(
          "Opposite product object was not found for product \"%1%\".",
          product.directId);
      throw ConnectError("Failed to build product list");
    }
  }

#if 0
     OrderRequest("CancelOrder", "order_id=4456419", m_settings, GetContext(),
                  GetLog(), GetTradingLog())
         .Send(m_tradingSession);
#endif

  m_pollingTask.AddTask(
      "Orders", 0,
      [this]() {
        UpdateOrders();
        return true;
      },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
  m_pollingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

  m_pollingTask.AccelerateNextPolling();
}

Volume ExcambiorexTradingSystem::CalcCommission(const Qty &qty,
                                                const Price &price,
                                                const OrderSide &,
                                                const trdk::Security &) const {
  return (qty * price) * (0.3 / 100);
}

void ExcambiorexTradingSystem::UpdateBalances() {
  SetBalances(boost::get<1>(m_balancesRequest.Send(m_pollingSession)));
}

bool ExcambiorexTradingSystem::UpdateOrders() {
  std::string requestList;
  size_t orderRequestListVersion;
  {
    const boost::mutex::scoped_lock lock(m_ordersRequestMutex);
    if (m_orderRequestList.empty()) {
      return false;
    }
    for (const auto *product : m_orderRequestList) {
      if (!requestList.empty()) {
        requestList.push_back(',');
      }
      requestList += product->directId;
    }
    orderRequestListVersion = m_orderRequestListVersion;
  }
  Assert(!requestList.empty());

  const auto &activeOrders = GetActiveOrderContextList();
  boost::unordered_set<const ExcambiorexProduct *> orderRequestList;
  boost::unordered_set<OrderId> actualIds;
  pt::ptime time;
  try {
    const auto response =
        ActiveOrdersRequest("pairs=" + requestList, m_settings, GetContext(),
                            GetLog(), GetTradingLog())
            .Send(m_pollingSession);
    time = boost::get<0>(response) - m_serverTimeDiff;
    for (const auto &node : boost::get<1>(response).get_child("orders")) {
      const auto &order = node.second;
      Verify(actualIds.emplace(order.get<OrderId>("id")).second);
      const auto &pair = order.get<std::string>("pair");
      const auto &products = m_products.get<ById>();
      const auto &product = products.find(pair);
      if (product == products.cend()) {
        boost::format error("Failed to resolve order pair \"%1%\"");
        error % pair;
        throw Exception(error.str().c_str());
      }
      orderRequestList.emplace(&*product);
    }
  } catch (const Exception &ex) {
    boost::format error("Failed to apply order update: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  } catch (const std::exception &ex) {
    boost::format error("Failed to read order: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  }

  for (const auto &context : activeOrders) {
    const auto &orderId = context->GetOrderId();
    if (actualIds.count(orderId)) {
      continue;
    }
    FinalizeOrder(time, orderId);
  }

  {
    const boost::mutex::scoped_lock lock(m_ordersRequestMutex);
    if (orderRequestListVersion == m_orderRequestListVersion) {
      orderRequestList.swap(m_orderRequestList);
    }
    return !m_orderRequestList.empty();
  }
}

void ExcambiorexTradingSystem::FinalizeOrder(const pt::ptime &time,
                                             const OrderId &id) {
  bool isCanceled = false;
  {
    const boost::mutex::scoped_lock lock(m_orderCancelingMutex);
    isCanceled = m_canceledOrders.count(id) != 0;
  }
  !isCanceled ? OnOrderFilled(time, id, boost::none)
              : OnOrderCanceled(time, id, boost::none, boost::none);
}

boost::optional<OrderCheckError> ExcambiorexTradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  {
    const auto result = Base::CheckOrder(security, currency, qty, price, side);
    if (result) {
      return result;
    }
  }
  return boost::none;
}

bool ExcambiorexTradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
ExcambiorexTradingSystem::SendOrderTransaction(
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

  const auto &products = m_products.get<BySymbol>();
  const auto &productIt = products.find(security.GetSymbol().GetSymbol());
  if (productIt == products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }
  const auto &product = *productIt;

  boost::format requestParams(
      "pair=%1%&type=%2%&amount=%3%&rate=%4%&feecoin=%5%&allowpartial=1");
  requestParams % product.directId                 // 1
      % (side == ORDER_SIDE_BUY ? "buy" : "sell")  // 2
      % qty                                        // 3
      % *price                                     // 4
      % security.GetSymbol().GetQuoteSymbol();     // 5

  OrderRequest request("SubmitOrder", requestParams.str(), m_settings,
                       GetContext(), GetLog(), GetTradingLog());

  const auto response = boost::get<1>(request.Send(m_tradingSession));

  SubsctibeAtOrderUpdates(
      (side == ORDER_SIDE_BUY &&
       product.buyCoinAlias == security.GetSymbol().GetBaseSymbol()) ||
              (side == ORDER_SIDE_SELL &&
               product.sellCoinAlias == security.GetSymbol().GetBaseSymbol())
          ? product
          : *product.oppositeProduct);
  SetBalances(response);

  try {
    return boost::make_unique<OrderTransactionContext>(
        *this, response.get<OrderId>("order_id"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void ExcambiorexTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  ptr::ptree response;
  {
    const boost::mutex::scoped_lock lock(m_orderCancelingMutex);
    response = boost::get<1>(
        OrderRequest("CancelOrder",
                     "order_id=" + boost::lexical_cast<std::string>(
                                       transaction.GetOrderId()),
                     m_settings, GetContext(), GetLog(), GetTradingLog())
            .Send(m_tradingSession));
    m_canceledOrders.emplace(transaction.GetOrderId());
  }
  SetBalances(response);
}

void ExcambiorexTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void ExcambiorexTradingSystem::SetBalances(const ptr::ptree &source) {
  boost::unordered_map<std::string, Volume> data;
  try {
    for (const auto &node : source.get_child("funds")) {
      Verify(data.emplace(node.first, node.second.get_value<Volume>()).second);
    }
    for (const auto &node : source.get_child("left")) {
      const auto &it = data.find(node.first);
      Assert(it != data.cend());
      if (it == data.cend()) {
        continue;
      }
      const auto &total = it->second;
      auto available = node.second.get_value<Volume>();
      AssertGe(total, available);
      auto locked = total - available;
      const auto &symbolIt = m_currencies.find(node.first);
      Assert(symbolIt != m_currencies.cend());
      const auto &symbol =
          symbolIt != m_currencies.cend() ? symbolIt->second : node.first;
      m_balances.Set(symbol, std::move(available), std::move(locked));
    }
  } catch (const std::exception &ex) {
    boost::format error("Failed to read balance: \"%1%\" (%2%)");
    error % ex.what()                      // 1
        % ConvertToString(source, false);  // 2
    throw CommunicationError(error.str().c_str());
  }
}

void ExcambiorexTradingSystem::SubsctibeAtOrderUpdates(
    const ExcambiorexProduct &product) {
  {
    const boost::mutex::scoped_lock lock(m_ordersRequestMutex);
    if (!m_orderRequestList.emplace(&product).second ||
        m_orderRequestList.size() > 1) {
      return;
    }
    ++m_orderRequestListVersion;
  }
  m_pollingTask.AddTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateExcambiorexTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<ExcambiorexTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
