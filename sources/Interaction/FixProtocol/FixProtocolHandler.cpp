/*******************************************************************************
 *   Created: 2017/10/01 05:23:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolHandler.hpp"
#include "Core/TradingLog.hpp"
#include "FixProtocolIncomingMessages.hpp"
#include "FixProtocolOutgoingMessages.hpp"
#include "FixProtocolSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace in = fix::Incoming;
namespace out = fix::Outgoing;
namespace pt = boost::posix_time;

namespace {
typedef NetworkStreamClient::ProtocolError ProtocolError;

fix::Settings LoadSettings(const IniSectionRef &conf,
                           const trdk::Settings &settings,
                           ModuleEventsLog &log) {
  fix::Settings result(conf, settings);
  result.Log(log);
  result.Validate();
  return result;
}
}

class Handler::Implementation : private boost::noncopyable {
 public:
  const fix::Settings m_settings;
  bool m_isAuthorized;
  out::StandardHeader m_standardHeader;

 public:
  explicit Implementation(const Context &context,
                          const Lib::IniSectionRef &conf,
                          ModuleEventsLog &log)
      : m_settings(LoadSettings(conf, context.GetSettings(), log)),
        m_isAuthorized(false),
        m_standardHeader(m_settings) {}
};

Handler::Handler(const Context &context,
                 const Lib::IniSectionRef &conf,
                 ModuleEventsLog &log)
    : m_pimpl(boost::make_unique<Implementation>(context, conf, log)) {}

Handler::~Handler() = default;

bool Handler::IsAuthorized() const { return m_pimpl->m_isAuthorized; }

const fix::Settings &Handler::GetSettings() const {
  return m_pimpl->m_settings;
}

out::StandardHeader &Handler::GetStandardOutgoingHeader() {
  return m_pimpl->m_standardHeader;
}

void Handler::OnLogon(const in::Logon &logon, NetworkStreamClient &) {
  if (m_pimpl->m_isAuthorized) {
    ProtocolError("Received unexpected Logon-message",
                  &*logon.GetMessageBegin(), 0);
  }
  m_pimpl->m_isAuthorized = true;
}

void Handler::OnLogout(const in::Logout &logout, NetworkStreamClient &client) {
  GetLog().Info("%1%Logout with the reason: \"%2%\".",
                client.GetLogTag(),  // 1
                logout.ReadText());  // 3
}

void Handler::OnHeartbeat(const in::Heartbeat &, NetworkStreamClient &client) {
  client.Send(out::Heartbeat(GetStandardOutgoingHeader()).Export(SOH));
}

void Handler::OnTestRequest(const in::TestRequest &testRequest,
                            NetworkStreamClient &client) {
  client.Send(Outgoing::Heartbeat(testRequest, GetStandardOutgoingHeader())
                  .Export(SOH));
}

void Handler::OnMarketDataSnapshotFullRefresh(
    const in::MarketDataSnapshotFullRefresh &message,
    NetworkStreamClient &,
    const Milestones &) {
  ProtocolError(
      "Current stream does not support Market Data Snapshot Full Refresh "
      "messages",
      &*message.GetMessageBegin(), 0);
}

void Handler::OnMarketDataIncrementalRefresh(
    const in::MarketDataIncrementalRefresh &message,
    NetworkStreamClient &,
    const Milestones &) {
  ProtocolError(
      "Current stream does not support Market Data Incremental Refresh "
      "messages",
      &*message.GetMessageBegin(), 0);
}
