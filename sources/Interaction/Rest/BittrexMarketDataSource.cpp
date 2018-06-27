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
#include "BittrexMarketDataSource.hpp"
#include "BittrexRequest.hpp"
#include "PollingTask.hpp"
#include "Security.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction::Rest;
namespace r = Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

BittrexMarketDataSource::BittrexMarketDataSource(const App &,
                                                 Context &context,
                                                 std::string instanceName,
                                                 std::string title,
                                                 const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_session(CreateSession("bittrex.com", m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

BittrexMarketDataSource::~BittrexMarketDataSource() {
  try {
    m_pollingTask.reset();
    // Each object, that implements CreateNewSecurityObject should wait for
    // log flushing before destroying objects:
    GetTradingLog().WaitForFlush();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void BittrexMarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");
  try {
    m_products = RequestBittrexProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
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

void BittrexMarketDataSource::SubscribeToSecurities() {
  const boost::mutex::scoped_lock lock(m_securitiesLock);
  for (auto &security : m_securities) {
    if (security.second) {
      continue;
    }
    const auto &product =
        m_products.find(security.first->GetSymbol().GetSymbol());
    Assert(product != m_products.cend());
    if (product == m_products.cend()) {
      continue;
    }
    security.second = std::make_unique<BittrexPublicRequest>(
        "getorderbook", "market=" + product->second.id + "&type=both",
        GetContext(), GetLog());
    security.first->SetTradingSessionState(pt::not_a_date_time, true);
  }
}

trdk::Security &BittrexMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &product = m_products.find(symbol.GetSymbol());
  if (product == m_products.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }

  const boost::mutex::scoped_lock lock(m_securitiesLock);
  m_securities.emplace_back(
      boost::make_shared<Rest::Security>(GetContext(), symbol, *this,
                                         Rest::Security::SupportedLevel1Types()
                                             .set(LEVEL1_TICK_BID_PRICE)
                                             .set(LEVEL1_TICK_BID_QTY)
                                             .set(LEVEL1_TICK_ASK_PRICE)
                                             .set(LEVEL1_TICK_ASK_QTY)),
      nullptr);
  return *m_securities.back().first;
}

void BittrexMarketDataSource::UpdatePrices() {
  const boost::mutex::scoped_lock lock(m_securitiesLock);
  for (const auto &subscribtion : m_securities) {
    Rest::Security &security = *subscribtion.first;
    if (!subscribtion.second) {
      continue;
    }
    Request &request = *subscribtion.second;

    try {
      const auto &response = request.Send(m_session);
      UpdatePrices(boost::get<0>(response), boost::get<1>(response), security,
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
  if (source) {
    for (const auto &level : *source) {
      return std::make_pair(
          Level1TickValue::Create<qtyType>(level.second.get<Qty>("Quantity")),
          Level1TickValue::Create<priceType>(level.second.get<Price>("Rate")));
    }
  }
  return boost::none;
}
#pragma warning(pop)
}
void BittrexMarketDataSource::UpdatePrices(const pt::ptime &time,
                                           const ptr::ptree &source,
                                           r::Security &security,
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

boost::shared_ptr<MarketDataSource> CreateBittrexMarketDataSource(
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  return boost::make_shared<BittrexMarketDataSource>(
      App::GetInstance(), context, std::move(instanceName), std::move(title),
      configuration);
}

////////////////////////////////////////////////////////////////////////////////
