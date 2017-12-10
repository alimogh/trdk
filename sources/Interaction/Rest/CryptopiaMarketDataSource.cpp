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

CryptopiaMarketDataSource::CryptopiaMarketDataSource(
    const App &,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf)
    : Base(context, instanceName),
      m_settings(conf, GetLog()),
      m_session("www.cryptopia.co.nz"),
      m_pullingTask(boost::make_unique<PullingTask>(m_settings.pullingSetttings,
                                                    GetLog())) {
  m_session.setKeepAlive(true);
}

CryptopiaMarketDataSource::~CryptopiaMarketDataSource() {
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

void CryptopiaMarketDataSource::Connect(const IniSectionRef &) {
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

  std::vector<std::string> uriSymbolsPath;
  uriSymbolsPath.reserve(m_securities.size());
  for (const auto &security : m_securities) {
    uriSymbolsPath.emplace_back(
        boost::lexical_cast<std::string>(security.first));
  }
  const auto &request = boost::make_shared<CryptopiaPublicRequest>(
      "GetMarketOrderGroups", boost::join(uriSymbolsPath, "-") + "/1");

  m_pullingTask->ReplaceTask(
      "Prices", 1,
      [this, request]() {
        RequestActualPrices(*request);
        return true;
      },
      m_settings.pullingSetttings.GetPricesRequestFrequency());
  m_pullingTask->AccelerateNextPulling();
}

trdk::Security &CryptopiaMarketDataSource::CreateNewSecurityObject(
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

void CryptopiaMarketDataSource::RequestActualPrices(Request &request) {
  try {
    const auto &response = request.Send(m_session, GetContext());
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
      UpdatePrices(time, pairNode, *security->second, delayMeasurement);
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
std::pair<Level1TickValue, Level1TickValue> ReadTopPrice(const Source &source) {
  if (source) {
    for (const auto &level : *source) {
      return {
          Level1TickValue::Create<priceType>(level.second.get<Price>("Price")),
          Level1TickValue::Create<qtyType>(level.second.get<Qty>("Total"))};
    }
  }
  return {Level1TickValue::Create<priceType>(
              std::numeric_limits<double>::quiet_NaN()),
          Level1TickValue::Create<qtyType>(
              std::numeric_limits<double>::quiet_NaN())};
}
#pragma warning(pop)
}
void CryptopiaMarketDataSource::UpdatePrices(
    const pt::ptime &time,
    const ptr::ptree &source,
    r::Security &security,
    const Milestones &delayMeasurement) {
  const auto &bid = ReadTopPrice<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
      source.get_child_optional("Buy"));
  const auto &ask = ReadTopPrice<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
      source.get_child_optional("Sell"));
  security.SetLevel1(time, bid.first, bid.second, ask.first, ask.second,
                     delayMeasurement);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<MarketDataSource> CreateCryptopiaMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<CryptopiaMarketDataSource>(
      App::GetInstance(), context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
