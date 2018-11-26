//
//    Created: 2018/07/26 10:10 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Session.hpp"

using namespace trdk;
using namespace Lib::TimeMeasurement;
using namespace Interaction;
using namespace Huobi;
using namespace Rest;
using namespace Lib;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

Huobi::MarketDataSource::MarketDataSource(const App &,
                                          Context &context,
                                          std::string instanceName,
                                          std::string title,
                                          const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()) {}

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &conf) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  return boost::make_shared<Huobi::MarketDataSource>(
      App::GetInstance(), context, std::move(instanceName), std::move(title),
      conf);
}

void Huobi::MarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");

  boost::unordered_map<std::string, Product> products;
  {
    auto session = CreateSession(m_settings, false);
    try {
      products = RequestProductList(session, GetContext(), GetLog());
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }
  }

  boost::unordered_set<std::string> symbolListHint;
  for (const auto &product : products) {
    symbolListHint.insert(product.first);
  }

  Base::Connect();

  m_products = std::move(products);
  m_symbolListHint = std::move(symbolListHint);
}

trdk::Security &Huobi::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products.find(symbol.GetSymbol());
  if (product == m_products.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }
  const auto &result = m_securities.emplace(
      product->second.id,
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)));
  Assert(result.second);
  result.first->second->SetTradingSessionState(pt::not_a_date_time, true);
  return *result.first->second;
}

std::unique_ptr<Huobi::MarketDataSource::Connection>
Huobi::MarketDataSource::CreateConnection() const {
  class Connection : public TradingLib::WebSocketConnection {
   public:
    typedef WebSocketConnection Base;

    explicit Connection(
        const boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
            &subscription)
        : WebSocketConnection("api.huobi.pro"), m_subscription(subscription) {}
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
      Handshake("/ws/");
      Start(events);
      for (const auto &security : m_subscription) {
        ptr::ptree request;
        request.add("sub", "market." + security.first + ".depth.step0");
        request.add("id", security.first);
        Write(request);
      }
    }

   protected:
    ptr::ptree ParseJson(std::istream &is) const override {
      namespace ios = boost::iostreams;
      ios::filtering_istream filter;
      filter.push(ios::gzip_decompressor{});
      filter.push(is);
      return Base::ParseJson(filter);
    }

    const boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
        &m_subscription;
  };

  return boost::make_unique<Connection>(m_securities);
}

void Huobi::MarketDataSource::HandleMessage(
    const pt::ptime &,
    const ptr::ptree &message,
    const Milestones &delayMeasurement) {
  const auto &channel = message.get_optional<std::string>("ch");
  if (channel) {
    auto symbol = channel->substr(7);
    symbol = symbol.substr(0, symbol.find('.'));
    const auto &security = m_securities.find(symbol);
    if (security == m_securities.cend()) {
      throw Exception("Unknown symbol");
    }
    UpdatePrices(
        ConvertToPTimeFromMicroseconds(message.get<uintmax_t>("ts") * 1000) -
            m_serverTimeDiff,
        message.get_child("tick"), *security->second, delayMeasurement);
    return;
  }
  if (message.get_optional<uintmax_t>("ping")) {
    Send(message);
  }
}

namespace {

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopPrice(
    const Source &source) {
  if (!source) {
    return boost::none;
  }
  boost::optional<Level1TickValue> price;
  boost::optional<Level1TickValue> qty;
  for (const auto &level : *source) {
    for (const auto &node : level.second) {
      const auto &value = node.second.template get_value<Double>();
      Assert(!qty);
      if (!price) {
        price = Level1TickValue::Create<priceType>(value);
      } else {
        qty = Level1TickValue::Create<qtyType>(value);
        break;
      }
    }
    break;
  }
  if (!qty) {
    Assert(!price);
    return boost::none;
  }
  Assert(price);
  return std::make_pair(*price, *qty);
}
#pragma warning(pop)

}  // namespace

void Huobi::MarketDataSource::UpdatePrices(const pt::ptime &time,
                                           const ptr::ptree &source,
                                           Rest::Security &security,
                                           const Milestones &delayMeasurement) {
  const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("bids"));
  const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("asks"));

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

const boost::unordered_set<std::string>
    &Huobi::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}
