/*******************************************************************************
 *   Created: 2017/12/07 15:24:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaMarketDataSource.hpp"
#include "CryptopiaRequest.hpp"
#include "PollingTask.hpp"
#include "Security.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;
using namespace Interaction::Rest;
namespace r = Interaction::Rest;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

CryptopiaMarketDataSource::CryptopiaMarketDataSource(const App &,
                                                     Context &context,
                                                     std::string instanceName,
                                                     std::string title,
                                                     const ptr::ptree &conf)
    : Base(context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_session(CreateSession("www.cryptopia.co.nz", m_settings, false)),
      m_pollingTask(boost::make_unique<PollingTask>(m_settings.pollingSetttings,
                                                    GetLog())) {}

CryptopiaMarketDataSource::~CryptopiaMarketDataSource() {
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

void CryptopiaMarketDataSource::Connect() {
  GetLog().Debug("Creating connection...");
  try {
    m_products = RequestCryptopiaProductList(m_session, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

void CryptopiaMarketDataSource::SubscribeToSecurities() {
  if (m_securities.empty()) {
    return;
  }

  const auto maxSymbolsPerRequest = 5;

  auto requests = boost::make_shared<
      std::vector<boost::shared_ptr<CryptopiaPublicRequest>>>();
  requests->reserve((m_securities.size() / maxSymbolsPerRequest) + 1);
  for (auto securityIt = m_securities.begin();
       securityIt != m_securities.cend();) {
    static_assert(maxSymbolsPerRequest > 0,
                  "Empty request and loop without end.");
    std::vector<std::string> uriSymbolsPath;
    uriSymbolsPath.reserve(maxSymbolsPerRequest);
    while (uriSymbolsPath.size() < maxSymbolsPerRequest &&
           securityIt != m_securities.cend()) {
      uriSymbolsPath.emplace_back(
          boost::lexical_cast<std::string>(securityIt->first));
      ++securityIt;
    }
    if (uriSymbolsPath.empty()) {
      Assert(securityIt == m_securities.cend());
      break;
    }
    requests->emplace_back(boost::make_shared<CryptopiaPublicRequest>(
        "GetMarketOrderGroups", boost::join(uriSymbolsPath, "-") + "/1",
        GetContext(), GetLog()));
  }
  Assert(!requests->empty());
  requests->shrink_to_fit();

  m_pollingTask->ReplaceTask(
      "Prices", 1,
      [this, requests]() {
        for (const auto &request : *requests) {
          UpdatePrices(*request);
        }
        return true;
      },
      m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
  m_pollingTask->AccelerateNextPolling();
}

trdk::Security &CryptopiaMarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &productIndex = m_products.get<BySymbol>();
  const auto &product = productIndex.find(symbol.GetSymbol());
  if (product == productIndex.cend()) {
    boost::format message("Symbol \"%1%\" is not in the exchange product list");
    message % symbol.GetSymbol();
    throw SymbolIsNotSupportedException(message.str().c_str());
  }

  {
    const auto &it = m_securities.find(product->id);
    if (it != m_securities.cend()) {
      return *it->second.second;
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

  Verify(m_securities.emplace(product->id, std::make_pair(product, result))
             .second);

  return *result;
}

void CryptopiaMarketDataSource::UpdatePrices(Request &request) {
  try {
    const auto &response = request.Send(m_session);
    const auto &time = boost::get<0>(response);
    const auto &delayMeasurement = boost::get<2>(response);
    for (const auto &record : boost::get<1>(response)) {
      const auto &pairNode = record.second;
      const auto security =
          m_securities.find(pairNode.get<CryptopiaProductId>("TradePairId"));
      if (security == m_securities.cend()) {
        GetLog().Error("Received price for unknown product ID \"%1%\".",
                       pairNode.get<std::string>("TradePairId"));
        continue;
      }
      UpdatePrices(time, pairNode, *security->second.first,
                   *security->second.second, delayMeasurement);
    }
  } catch (const std::exception &ex) {
    for (auto &security : m_securities) {
      try {
        security.second.second->SetOnline(pt::not_a_date_time, false);
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
  if (source) {
    for (const auto &level : *source) {
      return std::make_pair(
          Level1TickValue::Create<priceType>(level.second.get<Price>("Price")),
          Level1TickValue::Create<qtyType>(level.second.get<Qty>("Total")));
    }
  }
  return boost::none;
}
#pragma warning(pop)

template <Level1TickType priceType, Level1TickType qtyType>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> Reverse(
    const std::pair<Level1TickValue, Level1TickValue> &source,
    const trdk::Security &security) {
  Assert(source.first.GetType() == LEVEL1_TICK_BID_PRICE ||
         source.first.GetType() == LEVEL1_TICK_ASK_PRICE);
  AssertNe(priceType, source.first.GetType());
  Assert(source.second.GetType() == LEVEL1_TICK_BID_QTY ||
         source.second.GetType() == LEVEL1_TICK_ASK_QTY);
  AssertNe(qtyType, source.second.GetType());
  return std::make_pair(
      Level1TickValue::Create<priceType>(
          ReversePrice(source.first.GetValue(), security)),
      Level1TickValue::Create<qtyType>(ReverseQty(
          source.first.GetValue(), source.second.GetValue(), security)));
}
}
void CryptopiaMarketDataSource::UpdatePrices(
    const pt::ptime &time,
    const ptr::ptree &source,
    const CryptopiaProduct &product,
    r::Security &security,
    const Milestones &delayMeasurement) {
  auto bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("Buy"));
  auto ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("Sell"));

  if (product.isReversed) {
    bid.swap(ask);
    if (bid) {
      bid = Reverse<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(*bid, security);
    }
    if (ask) {
      ask = Reverse<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(*ask, security);
    }
  }

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

boost::shared_ptr<MarketDataSource> CreateCryptopiaMarketDataSource(
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  return boost::make_shared<CryptopiaMarketDataSource>(
      App::GetInstance(), context, std::move(instanceName), std::move(title),
      configuration);
}

////////////////////////////////////////////////////////////////////////////////
