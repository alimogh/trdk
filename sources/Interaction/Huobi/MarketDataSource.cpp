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
#include "Request.hpp"
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
      m_settings(conf, GetLog()),
      m_session(CreateSession(m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

Huobi::MarketDataSource::~MarketDataSource() {
  try {
    m_pollingTask.reset();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
  // Each object, that implements CreateNewSecurityObject should wait for log
  // flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

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
  try {
    m_products = RequestProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

void Huobi::MarketDataSource::SubscribeToSecurities() {
  std::vector<
      std::pair<boost::shared_ptr<Rest::Security>, boost::shared_ptr<Request>>>
      requests;
  for (const auto &security : m_securities) {
    requests.emplace_back(std::make_pair(
        security.second,
        boost::make_shared<PublicRequest>(
            "market/depth", "symbol=" + security.first + "&type=step1",
            GetContext(), GetLog())));
  }
  m_pollingTask->ReplaceTask(
      "Prices", 1,
      [this, requests]() {
        for (const auto &request : requests) {
          UpdatePrices(*request.first, *request.second);
        }
        return true;
      },
      m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
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
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)));
  Assert(result.second);
  result.first->second->SetTradingSessionState(pt::not_a_date_time, true);
  return *result.first->second;
}

void Huobi::MarketDataSource::UpdatePrices(Rest::Security &security,
                                           Request &request) {
  try {
    const auto &response = request.Send(m_session);
    UpdatePrices(boost::get<1>(response), security, boost::get<2>(response));
  } catch (const std::exception &ex) {
    try {
      security.SetOnline(pt::not_a_date_time, false);
    } catch (...) {
      AssertFailNoException();
      throw;
    }
    boost::format error("Failed to read prices for %1%: \"%2%\"");
    error % security  // 1
        % ex.what();  // 2
    try {
      throw;
    } catch (const CommunicationError &) {
      throw CommunicationError(error.str().c_str());
    } catch (...) {
      throw Exception(error.str().c_str());
    }
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
  return std::make_pair(std::move(*price), std::move(*qty));
}
#pragma warning(pop)
}

void Huobi::MarketDataSource::UpdatePrices(const ptr::ptree &source,
                                           Rest::Security &security,
                                           const Milestones &delayMeasurement) {
  const auto &time =
      ConvertToPTimeFromMicroseconds(source.get<int64_t>("ts") * 1000);

  const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("tick.bids"));
  const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("tick.asks"));

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
