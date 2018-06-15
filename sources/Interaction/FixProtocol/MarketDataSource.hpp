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
  explicit MarketDataSource(Context&,
                            std::string instanceName,
                            std::string title,
                            const boost::property_tree::ptree&);
  ~MarketDataSource() override;

  Context& GetContext() override;
  const Context& GetContext() const override;

  ModuleEventsLog& GetLog() const override;

  const Security& GetSecurityByFixId(size_t) const;
  Security& GetSecurityByFixId(size_t);

  void Connect() override;
  void SubscribeToSecurities() override;
  void ResubscribeToSecurities();

  void OnConnectionRestored() override;

  void OnMarketDataSnapshotFullRefresh(
      const Incoming::MarketDataSnapshotFullRefresh&,
      Lib::NetworkStreamClient&,
      const Lib::TimeMeasurement::Milestones&) override;

 protected:
  trdk::Security& CreateNewSecurityObject(const Lib::Symbol&) override;

 private:
  Client m_client;
  boost::unordered_map<size_t, boost::shared_ptr<Security>> m_securities;
};
}  // namespace FixProtocol
}  // namespace Interaction
}  // namespace trdk
