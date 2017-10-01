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
#include "FixProtocolIncomingMessages.hpp"
#include "FixProtocolOutgoingMessages.hpp"
#include "FixProtocolSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace in = fix::Incoming;
namespace out = fix::Outgoing;
namespace gr = boost::gregorian;
namespace pt = boost::posix_time;

fix::MarketDataSource::MarketDataSource(size_t index,
                                        Context &context,
                                        const std::string &instanceName,
                                        const IniSectionRef &conf)
    : trdk::MarketDataSource(index, context, instanceName),
      Handler(context, conf, trdk::MarketDataSource::GetLog()),
      m_client("Prices", *this) {}

Context &fix::MarketDataSource::GetContext() {
  return trdk::MarketDataSource::GetContext();
}
const Context &fix::MarketDataSource::GetContext() const {
  return trdk::MarketDataSource::GetContext();
}

ModuleEventsLog &fix::MarketDataSource::GetLog() const {
  return trdk::MarketDataSource::GetLog();
}

void fix::MarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Connecting...");
  m_client.Connect();
  GetLog().Info("Connected.");
}

void fix::MarketDataSource::SubscribeToSecurities() {
  GetLog().Debug("Sending market data request for %1% securities...",
                 m_securities.size());

  try {
    for (const auto &security : m_securities) {
      if (!security.second->GetRequest()) {
        //         security->SetOnline(security->GetLastMarketDataTime(), true);
        //         security->SetTradingSessionState(security->GetLastMarketDataTime(),
        //                                          true);
      }
      GetLog().Info("Sending Market Data Request for \"%1%\" (%2%)...",
                    *security.second,              // 1
                    security.second->GetFixId());  // 2
      m_client.Send(out::MarketDataRequest(*security.second,
                                           GetStandardOutgoingHeader()));
    }
  } catch (const Exception &ex) {
    GetLog().Error("Failed to send market data request: \"%1%\".", ex);
    throw Error("Failed to send market data request");
  }

  GetLog().Debug("Market data request sent.");
}

trdk::Security &fix::MarketDataSource::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &result =
      boost::make_shared<fix::Security>(GetContext(), symbol, *this,
                                        Security::SupportedLevel1Types()
                                            .set(LEVEL1_TICK_BID_PRICE)
                                            .set(LEVEL1_TICK_ASK_PRICE));
  Verify(m_securities.emplace(result->GetFixId(), result).second);
  return *result;
}

const fix::Security &fix::MarketDataSource::GetSecurityByFixId(
    size_t id) const {
  return const_cast<MarketDataSource *>(this)->GetSecurityByFixId(id);
}

fix::Security &fix::MarketDataSource::GetSecurityByFixId(size_t id) {
  const auto &result = m_securities.find(id);
  if (result == m_securities.cend()) {
    boost::format error("Failed to resolve security with FIX Symbol ID %1%");
    error % id;
    throw Error(error.str().c_str());
  }
  return *result->second;
}

void fix::MarketDataSource::OnConnectionRestored() {
  throw MethodDoesNotImplementedError(
      "fix::MarketDataSource::OnConnectionRestored is not implemented");
}

void fix::MarketDataSource::OnMarketDataSnapshotFullRefresh(
    const in::MarketDataSnapshotFullRefresh &snapshot,
    NetworkStreamClient &,
    const Milestones &delayMeasurement) {
  auto &security = snapshot.ReadSymbol(*this);
  bool isPreviouslyChanged = false;
  snapshot.ReadEachMarketDataEntity([&](Level1TickValue &&value, bool isLast) {
    return security.AddLevel1Tick(snapshot.GetTime(), std::move(value), isLast,
                                  isPreviouslyChanged, delayMeasurement);
  });
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
