/*******************************************************************************
 *   Created: 2017/09/24 03:50:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {
class Handler : private boost::noncopyable {
 public:
  explicit Handler(const Context &,
                   const Lib::IniSectionRef &,
                   ModuleEventsLog &);
  virtual ~Handler();

 public:
  const Settings &GetSettings() const;
  virtual Context &GetContext() = 0;
  virtual const Context &GetContext() const = 0;
  virtual ModuleEventsLog &GetLog() const = 0;
  Outgoing::StandardHeader &GetStandardOutgoingHeader();

 public:
  bool IsAuthorized() const;

 public:
  virtual void OnConnectionRestored() = 0;

  void OnLogon(const Incoming::Logon &, Lib::NetworkStreamClient &);
  void OnLogout(const Incoming::Logout &, Lib::NetworkStreamClient &);

  void OnHeartbeat(const Incoming::Heartbeat &, Lib::NetworkStreamClient &);
  void OnTestRequest(const Incoming::TestRequest &, Lib::NetworkStreamClient &);

  virtual void OnResendRequest(const Incoming::ResendRequest &,
                               Lib::NetworkStreamClient &);
  virtual void OnReject(const Incoming::Reject &, Lib::NetworkStreamClient &);

  virtual void OnMarketDataSnapshotFullRefresh(
      const Incoming::MarketDataSnapshotFullRefresh &,
      Lib::NetworkStreamClient &,
      const Lib::TimeMeasurement::Milestones &);
  virtual void OnMarketDataIncrementalRefresh(
      const Incoming::MarketDataIncrementalRefresh &,
      Lib::NetworkStreamClient &,
      const Lib::TimeMeasurement::Milestones &);

  virtual void OnBusinessMessageReject(
      const Incoming::BusinessMessageReject &,
      Lib::NetworkStreamClient &,
      const Lib::TimeMeasurement::Milestones &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}