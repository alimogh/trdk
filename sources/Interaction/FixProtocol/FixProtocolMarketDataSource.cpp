/*******************************************************************************
 *   Created: 2017/09/19 19:55:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolMarketDataSource.hpp"
#include "FixProtocolSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace gr = boost::gregorian;

fix::MarketDataSource::MarketDataSource(size_t index,
                                        Context &context,
                                        const std::string &instanceName,
                                        const IniSectionRef &conf)
    : Base(index, context, instanceName),
      m_settings(conf),
      m_client("Prices", *this) {
  m_settings.Log(GetLog());
  m_settings.Validate();
}

void fix::MarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Connecting to the stream...");
  m_client.Connect();
  GetLog().Info("Connected to the stream.");
}

void fix::MarketDataSource::SubscribeToSecurities() {
  GetLog().Debug("Sending market data request for %1% securities...",
                 m_securities.size());

  try {
    for (const auto &security : m_securities) {
      if (!security->GetRequest()) {
        //         security->SetOnline(security->GetLastMarketDataTime(), true);
        //         security->SetTradingSessionState(security->GetLastMarketDataTime(),
        //                                          true);
      }
      m_client.RequestMarketData(*security);
    }
  } catch (const Exception &ex) {
    GetLog().Error("Failed to send market data request: \"%1%\".", ex);
    throw Error("Failed to send market data request");
  }

  GetLog().Debug("Market data request sent.");
}

void fix::MarketDataSource::ResubscribeToSecurities() {
  throw MethodDoesNotImplementedError(
      "fix::MarketDataSource::ResubscribeToSecurities is not implemented");
}

trdk::Security &fix::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  m_securities.emplace_back(
      boost::make_shared<fix::Security>(GetContext(), symbol, *this,
                                        Security::SupportedLevel1Types()
                                            .set(LEVEL1_TICK_BID_PRICE)
                                            .set(LEVEL1_TICK_ASK_PRICE)));
  return *m_securities.back();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<fix::MarketDataSource>(index, context, instanceName,
                                                   configuration);
}

////////////////////////////////////////////////////////////////////////////////
