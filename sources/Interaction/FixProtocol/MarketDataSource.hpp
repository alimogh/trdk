/*******************************************************************************
 *   Created: 2017/09/19 19:43:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Client.hpp"
#include "Handler.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {

class MarketDataSource : public trdk::MarketDataSource, public Handler {
 public:
  explicit MarketDataSource(Context &,
                            const std::string &instanceName,
                            const Lib::IniSectionRef &);
  virtual ~MarketDataSource() override;

 public:
  Context &GetContext() override;
  const Context &GetContext() const override;

  virtual ModuleEventsLog &GetLog() const override;

  const FixProtocol::Security &GetSecurityByFixId(size_t) const;
  FixProtocol::Security &GetSecurityByFixId(size_t);

 public:
  virtual void Connect(const trdk::Lib::IniSectionRef &) override;
  virtual void SubscribeToSecurities() override;
  void ResubscribeToSecurities();

 public:
  virtual void OnConnectionRestored() override;

  virtual void OnMarketDataSnapshotFullRefresh(
      const Incoming::MarketDataSnapshotFullRefresh &,
      Lib::NetworkStreamClient &,
      const Lib::TimeMeasurement::Milestones &) override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  Client m_client;
  boost::unordered_map<size_t, boost::shared_ptr<FixProtocol::Security>>
      m_securities;
};
}
}
}