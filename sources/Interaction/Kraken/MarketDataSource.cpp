//
//    Created: 2018/10/11 08:24
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
using namespace Kraken;
using namespace Rest;
using namespace Lib;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

Kraken::MarketDataSource::MarketDataSource(const App &,
                                           Context &context,
                                           std::string instanceName,
                                           std::string title,
                                           const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_session(CreateSession(m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSettings,
                                                    GetLog())) {}

Kraken::MarketDataSource::~MarketDataSource() {
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
  return boost::make_shared<Kraken::MarketDataSource>(
      App::GetInstance(), context, std::move(instanceName), std::move(title),
      conf);
}

void Kraken::MarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");

  boost::unordered_map<std::string, Product> products;
  try {
    products = RequestProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  boost::unordered_set<std::string> symbolListHint;
  for (const auto &product : products) {
    symbolListHint.insert(product.first);
  }

  m_products = std::move(products);
  m_symbolListHint = std::move(symbolListHint);
}

void Kraken::MarketDataSource::SubscribeToSecurities() {
  std::vector<std::pair<std::pair<ProductId, boost::shared_ptr<Rest::Security>>,
                        boost::shared_ptr<Request>>>
      requests;
  for (const auto &security : m_securities) {
    requests.emplace_back(std::make_pair(
        security, boost::make_shared<PublicRequest>(
                      "Depth", "pair=" + security.first + "&count=1",
                      GetContext(), GetLog())));
  }
  requests.shrink_to_fit();
  m_pollingTask->ReplaceTask(
      "Prices", 1,
      [this, requests]() {
        for (const auto &request : requests) {
          UpdatePrices(request.first.first, *request.first.second,
                       *request.second);
        }
        return true;
      },
      m_settings.pollingSettings.GetPricesRequestFrequency(), false);
}

trdk::Security &Kraken::MarketDataSource::CreateNewSecurityObject(
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

const boost::unordered_set<std::string>
    &Kraken::MarketDataSource::GetSymbolListHint() const {
  return m_symbolListHint;
}

void Kraken::MarketDataSource::UpdatePrices(const ProductId &productId,
                                            Rest::Security &security,
                                            Rest::Request &request) {
  try {
    const auto &response = request.Send(m_session);
    const auto &time = boost::get<0>(response);
    const auto &delayMeasurement = boost::get<2>(response);
    UpdatePrices(time, boost::get<1>(response).get_child(productId), security,
                 delayMeasurement);
  } catch (const std::exception &ex) {
    try {
      security.SetOnline(pt::not_a_date_time, false);
    } catch (...) {
      AssertFailNoException();
      throw;
    }
    boost::format error("Failed to read prices: \"%1%\"");
    error % ex.what();
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
  return std::make_pair(*price, *qty);
}
#pragma warning(pop)
}  // namespace

void Kraken::MarketDataSource::UpdatePrices(
    const pt::ptime &time,
    const ptr::ptree &source,
    Rest::Security &security,
    const Milestones &delayMeasurement) {
  try {
    const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
        source.get_child_optional("asks"));
    const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
        source.get_child_optional("bids"));

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
  } catch (const std::exception &ex) {
    boost::format error(R"(Failed to read order book: "%1%" ("%2%").)");
    error % ex.what()                           // 1
        % Lib::ConvertToString(source, false);  // 2
    throw Exception(error.str().c_str());
  }
}
