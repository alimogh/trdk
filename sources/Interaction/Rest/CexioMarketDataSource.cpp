/*******************************************************************************
 *   Created: 2018/01/18 00:25:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CexioMarketDataSource.hpp"
#include "CexioRequest.hpp"
#include "PollingTask.hpp"
#include "Security.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::Rest;

namespace r = trdk::Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

CexioMarketDataSource::CexioMarketDataSource(const App &,
                                             Context &context,
                                             const std::string &instanceName,
                                             const IniSectionRef &conf)
    : Base(context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_session(CreateSession("cex.io", m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

CexioMarketDataSource::~CexioMarketDataSource() {
  try {
    m_pollingTask.reset();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
  // Each object, that implements CreateNewSecurityObject should wait for
  // log flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void CexioMarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Creating connection...");
  try {
    m_products = RequestCexioProductList(*m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

void CexioMarketDataSource::SubscribeToSecurities() {
  if (m_products.empty()) {
    return;
  }

  {
    const boost::mutex::scoped_lock lock(m_securitiesMutex);
    for (auto &subscribtion : m_securities) {
      if (subscribtion.second.request) {
        continue;
      }
      subscribtion.second.request = boost::make_shared<CexioPublicRequest>(
          "order_book/" + subscribtion.first->id, "{depth: 1}", GetContext(),
          GetLog());
    }
  }

  m_pollingTask->AddTask(
      "Prices", 1,
      [this]() {
        UpdatePrices();
        return true;
      },
      m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
  m_pollingTask->AccelerateNextPolling();
}

trdk::Security &CexioMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products.find(symbol.GetSymbol());
  if (product == m_products.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }

  // Two locks as only one thread can add new securities.

  {
    const boost::mutex::scoped_lock lock(m_securitiesMutex);
    const auto &it = m_securities.find(&product->second);
    if (it != m_securities.cend()) {
      return *it->second.security;
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

  {
    const boost::mutex::scoped_lock lock(m_securitiesMutex);
    Verify(m_securities.emplace(&product->second, Subscribtion{result}).second);
  }

  return *result;
}

void CexioMarketDataSource::UpdatePrices() {
  const boost::mutex::scoped_lock lock(m_securitiesMutex);

  for (auto &subscribtion : m_securities) {
    if (!subscribtion.second.request) {
      continue;
    }
    auto &security = *subscribtion.second.security;
    auto &request = *subscribtion.second.request;

    try {
      const auto &response = request.Send(*m_session);
      UpdatePrices(boost::get<1>(response), subscribtion.second,
                   boost::get<2>(response));
    } catch (const std::exception &ex) {
      try {
        security.SetOnline(pt::not_a_date_time, false);
      } catch (...) {
        AssertFailNoException();
        throw;
      }
      boost::format error("Failed to read order book for \"%1%\": \"%2%\"");
      error % security  // 1
          % ex.what();  // 2
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

void CexioMarketDataSource::UpdatePrices(const ptr::ptree &source,
                                         Subscribtion &subscribtion,
                                         const Milestones &delayMeasurement) {
  try {
    const auto &id = source.get<decltype(subscribtion.lastBookId)>("id");
    if (id == subscribtion.lastBookId) {
      AssertLt(subscribtion.lastBookId, id);
      return;
    }

    const auto &time = ParseTimeStamp(source);

    const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
        source.get_child_optional("bids"));
    const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
        source.get_child_optional("asks"));

    if (bid && ask) {
      subscribtion.security->SetLevel1(time, bid->first, bid->second,
                                       ask->first, ask->second,
                                       delayMeasurement);
      subscribtion.security->SetOnline(pt::not_a_date_time, true);
    } else {
      subscribtion.security->SetOnline(pt::not_a_date_time, false);
      if (bid) {
        subscribtion.security->SetLevel1(time, bid->first, bid->second,
                                         delayMeasurement);
      } else if (ask) {
        subscribtion.security->SetLevel1(time, ask->first, ask->second,
                                         delayMeasurement);
      }
    }

    subscribtion.lastBookId = std::move(id);

  } catch (const std::exception &ex) {
    boost::format error("Failed to read order book: \"%1%\" (\"%2%\").");
    error % ex.what()                      // 1
        % ConvertToString(source, false);  // 2
    throw Exception(error.str().c_str());
  }
}

pt::ptime CexioMarketDataSource::ParseTimeStamp(const ptr::ptree &source) {
  return ParseCexioTimeStamp(source, m_serverTimeDiff);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateCexioMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<CexioMarketDataSource>(App::GetInstance(), context,
                                                   instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
