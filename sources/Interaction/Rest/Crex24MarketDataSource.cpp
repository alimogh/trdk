/*******************************************************************************
 *   Created: 2018/02/11 18:00:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crex24MarketDataSource.hpp"
#include "Crex24Request.hpp"
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

Crex24MarketDataSource::Crex24MarketDataSource(const App &,
                                               Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf)
    : Base(context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_session(CreateCrex24Session(m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

Crex24MarketDataSource::~Crex24MarketDataSource() {
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

void Crex24MarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Creating connection...");
  boost::unordered_map<std::string, Crex24Product> products;
  try {
    products = RequestCrex24ProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
  boost::unordered_map<std::string,
                       std::pair<Crex24Product, boost::shared_ptr<Security>>>
      securities;
  for (const auto &product : products) {
    securities.emplace(product.first, std::make_pair(product.second, nullptr));
  }
  securities.swap(m_securities);
}

void Crex24MarketDataSource::SubscribeToSecurities() {
  std::vector<
      std::pair<boost::shared_ptr<Security>, boost::shared_ptr<Request>>>
      subscribtion;
  for (const auto &security : m_securities) {
    if (!security.second.second) {
      continue;
    }
    auto request = boost::make_shared<Crex24PublicRequest>(
        "ReturnOrderBook",
        "request=[PairName=" + security.second.first.id + "]", GetContext(),
        GetLog());
    subscribtion.emplace_back(std::make_pair(security.second.second, request));
  }
  m_pollingTask->ReplaceTask(
      "Prices", 1,
      [this, subscribtion]() {
        UpdatePrices(subscribtion);
        return true;
      },
      m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
}

trdk::Security &Crex24MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &security = m_securities.find(symbol.GetSymbol());
  if (security == m_securities.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }
  if (security->second.second) {
    return *security->second.second;
  }
  security->second.second =
      boost::make_shared<r::Security>(GetContext(), symbol, *this,
                                      r::Security::SupportedLevel1Types()
                                          .set(LEVEL1_TICK_BID_PRICE)
                                          .set(LEVEL1_TICK_BID_QTY)
                                          .set(LEVEL1_TICK_ASK_PRICE)
                                          .set(LEVEL1_TICK_BID_QTY));
  security->second.second->SetTradingSessionState(pt::not_a_date_time, true);
  return *security->second.second;
}

void Crex24MarketDataSource::UpdatePrices(
    const std::vector<std::pair<boost::shared_ptr<r::Security>,
                                boost::shared_ptr<Request>>> &subscribtion) {
  for (const auto &security : subscribtion) {
    try {
      UpdatePrices(*security.first, *security.second);
    } catch (const std::exception &ex) {
      for (auto &offlineSecurity : subscribtion) {
        try {
          offlineSecurity.first->SetOnline(pt::not_a_date_time, false);
        } catch (...) {
          AssertFailNoException();
          throw;
        }
      }
      boost::format error("Failed to read \"depth\": \"%1%\"");
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
}

namespace {
#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType, typename Source>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopOfBook(
    const Source &source) {
  if (!source) {
    return boost::none;
  }
  for (const auto &node : *source) {
    const auto &level = node.second;
    return std::make_pair(
        Level1TickValue::Create<priceType>(level.get<Price>("CoinPrice")),
        Level1TickValue::Create<qtyType>(level.get<Qty>("CoinCount")));
  }
  return boost::none;
}
#pragma warning(pop)
}  // namespace

void Crex24MarketDataSource::UpdatePrices(r::Security &security,
                                          Request &request) {
  const auto &response = request.Send(m_session);
  const auto &time = boost::get<0>(response);
  const auto &data = boost::get<1>(response);
  const auto &delayMeasurement = boost::get<2>(response);
  const auto &bestAsk =
      ReadTopOfBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
          data.get_child_optional("BuyOrders"));
  const auto &bestBid =
      ReadTopOfBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
          data.get_child_optional("SellOrders"));
  if (bestAsk && bestBid) {
    security.SetLevel1(time, bestBid->first, bestBid->second, bestAsk->first,
                       bestAsk->second, delayMeasurement);
    security.SetOnline(pt::not_a_date_time, true);
  } else {
    security.SetOnline(pt::not_a_date_time, false);
    if (bestBid) {
      security.SetLevel1(time, bestBid->first, bestBid->second,
                         delayMeasurement);
    } else if (bestAsk) {
      security.SetLevel1(time, bestAsk->first, bestAsk->second,
                         delayMeasurement);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateCrex24MarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<Crex24MarketDataSource>(
      App::GetInstance(), context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
