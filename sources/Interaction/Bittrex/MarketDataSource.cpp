/*******************************************************************************
 *   Created: 2017/11/16 13:09:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Bittrex;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

Bittrex::MarketDataSource::MarketDataSource(const App &,
                                            Context &context,
                                            std::string instanceName,
                                            std::string title,
                                            const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()) {}

void Bittrex::MarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");

  const boost::unordered_map<std::string, Product> *products;

  {
    auto session = CreateSession("bittrex.com", m_settings, false);
    try {
      products = &GetProductList(session, GetContext(), GetLog());
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }
  }
  boost::unordered_set<std::string> symbolListHint;
  for (const auto &product : *products) {
    symbolListHint.insert(product.first);
  }

  Base::Connect();

  m_products = products;
  m_symbolListHint = std::move(symbolListHint);
}

const boost::unordered_set<std::string>
    &Bittrex::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}

trdk::Security &Bittrex::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products->find(symbol.GetSymbol());
  if (product == m_products->cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }

  const auto result = m_securities.emplace(
      product->second.id,
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_ASK_QTY)));
  return *result.first->second;
}

std::unique_ptr<Bittrex::MarketDataSource::Connection>
Bittrex::MarketDataSource::CreateConnection() const {
  class Connection : public TradingLib::WebSocketConnection {
   public:
    explicit Connection(
        const boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
            &subscription)
        : WebSocketConnection("socket.bittrex.com"),
          m_subscription(subscription) {}
    Connection(Connection &&) = default;
    Connection(const Connection &) = delete;
    Connection &operator=(Connection &&) = delete;
    Connection &operator=(const Connection &) = delete;
    ~Connection() override = default;

    void Connect() override { WebSocketConnection::Connect("https"); }

    void StartData(const Events &events) override {
      if (m_subscription.empty()) {
        return;
      }
      Handshake("/signalr");
      Start(events);
      for (const auto &security : m_subscription) {
        ptr::ptree request;
        request.add("req", "market." + security.first + ".depth.step1");
        request.add("id", security.first);
        Write(request);
      }
    }

   private:
    const boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
        &m_subscription;
  };

  return boost::make_unique<Connection>(m_securities);
}

void Bittrex::MarketDataSource::HandleMessage(const pt::ptime &,
                                              const ptr::ptree &message,
                                              const Milestones &) {
  GetLog().Debug("XXX %1%", Lib::ConvertToString(message, false));
}

namespace {

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopPrice(
    const Source &source) {
  if (source) {
    for (const auto &level : *source) {
      return std::make_pair(Level1TickValue::Create<qtyType>(
                                level.second.template get<Qty>("Quantity")),
                            Level1TickValue::Create<priceType>(
                                level.second.template get<Price>("Rate")));
    }
  }
  return boost::none;
}
#pragma warning(pop)
}  // namespace
void Bittrex::MarketDataSource::UpdatePrices(
    const pt::ptime &time,
    const ptr::ptree &source,
    Rest::Security &security,
    const Milestones &delayMeasurement) {
  const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("buy"));
  const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("sell"));
  if (bid && ask) {
    security.SetLevel1(time, bid->first, bid->second, ask->first, ask->second,
                       delayMeasurement);
    security.SetOnline(pt::not_a_date_time, true);
  } else {
    security.SetOnline(pt::not_a_date_time, false);
    if (bid) {
      security.SetLevel1(time, bid->first, bid->second, delayMeasurement);
    } else if (ask) {
      security.SetLevel1(time, ask->first, ask->second, delayMeasurement);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::MarketDataSource> CreateBittrexMarketDataSource(
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  return boost::make_shared<Bittrex::MarketDataSource>(
      App::GetInstance(), context, std::move(instanceName), std::move(title),
      configuration);
}

////////////////////////////////////////////////////////////////////////////////
