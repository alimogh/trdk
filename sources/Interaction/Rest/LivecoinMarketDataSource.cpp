/*******************************************************************************
 *   Created: 2017/12/12 19:24:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinMarketDataSource.hpp"
#include "PullingTask.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::Rest;

namespace r = trdk::Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

LivecoinMarketDataSource::LivecoinMarketDataSource(
    const App &,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf)
    : Base(context, instanceName),
      m_settings(conf, GetLog()),
      m_allOrderBooksRequest("/exchange/all/order_book",
                             "groupByPrice=true&depth=1"),
      m_session("api.livecoin.net"),
      m_pullingTask(boost::make_unique<PullingTask>(m_settings.pullingSetttings,
                                                    GetLog())) {
  m_session.setKeepAlive(true);
}

LivecoinMarketDataSource::~LivecoinMarketDataSource() {
  try {
    m_pullingTask.reset();
    // Each object, that implements CreateNewSecurityObject should wait for
    // log flushing before destroying objects:
    GetTradingLog().WaitForFlush();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void LivecoinMarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Creating connection...");
  try {
    m_products = RequestLivecoinProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

void LivecoinMarketDataSource::SubscribeToSecurities() {
  if (m_securities.empty()) {
    return;
  }

  std::vector<std::string> uriSymbolsPath;
  uriSymbolsPath.reserve(m_securities.size());
  for (const auto &security : m_securities) {
    uriSymbolsPath.emplace_back(
        boost::lexical_cast<std::string>(security.first));
  }

  m_pullingTask->AddTask(
      "Prices", 1,
      [this]() {
        UpdatePrices();
        return true;
      },
      m_settings.pullingSetttings.GetPricesRequestFrequency());
  m_pullingTask->AccelerateNextPulling();
}

trdk::Security &LivecoinMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products.find(symbol.GetSymbol());
  if (product == m_products.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedError(message.str().c_str());
  }

  {
    const auto &it = m_securities.find(product->second.id);
    if (it != m_securities.cend()) {
      return *it->second;
    }
  }

  const auto &result =
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY));
  result->SetTradingSessionState(pt::not_a_date_time, true);

  Verify(m_securities.emplace(product->second.id, result).second);

  return *result;
}

void LivecoinMarketDataSource::UpdatePrices() {
  try {
    const auto &response = m_allOrderBooksRequest.Send(m_session, GetContext());
    const auto &delayMeasurement = boost::get<2>(response);
    for (const auto &record : boost::get<1>(response)) {
      const auto security = m_securities.find(record.first);
      if (security == m_securities.cend()) {
        continue;
      }
      UpdatePrices(record.second, *security->second, delayMeasurement);
    }
  } catch (const std::exception &ex) {
    for (auto &security : m_securities) {
      try {
        security.second->SetOnline(pt::not_a_date_time, false);
      } catch (...) {
        AssertFailNoException();
        throw;
      }
    }
    boost::format error("Failed to read prices: \"%1%\"");
    error % ex.what();
    try {
      throw;
    } catch (const CommunicationError &) {
      throw CommunicationError(error.str().c_str());
    } catch (const Exception &) {
      throw Exception(error.str().c_str());
    } catch (const std::exception &) {
      throw CommunicationError(error.str().c_str());
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
      const auto &value = node.second.get_value<Double>();
      Assert(!qty);
      if (!price) {
        price = Level1TickValue::Create<priceType>(value);
      } else {
        qty = Level1TickValue::Create<qtyType>(value);
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
void LivecoinMarketDataSource::UpdatePrices(
    const ptr::ptree &source,
    r::Security &security,
    const Milestones &delayMeasurement) {
  const auto &time = pt::from_time_t(source.get<time_t>("timestamp"));
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
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateLivecoinMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<LivecoinMarketDataSource>(
      App::GetInstance(), context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
