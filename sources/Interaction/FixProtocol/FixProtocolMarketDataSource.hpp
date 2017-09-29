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

#include "Core/MarketDataSource.hpp"
#include "FixProtocolClient.hpp"
#include "FixProtocolSettings.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  typedef trdk::MarketDataSource Base;

 public:
  explicit MarketDataSource(size_t index,
                            Context &,
                            const std::string &instanceName,
                            const Lib::IniSectionRef &);
  virtual ~MarketDataSource() override = default;

 public:
  const Settings &GetSettings() const { return m_settings; }

  const FixProtocol::Security &GetSecurityByFixId(size_t) const;
  FixProtocol::Security &GetSecurityByFixId(size_t);

 public:
  virtual void Connect(const trdk::Lib::IniSectionRef &) override;
  virtual void SubscribeToSecurities() override;
  void ResubscribeToSecurities();

 public:
  bool OnMarketDataSnapshotFullRefresh(
      FixProtocol::Security &security,
      const boost::posix_time::ptime &,
      Level1TickValue &&,
      bool flush,
      bool isPreviouslyChanged,
      const Lib::TimeMeasurement::Milestones &);

 protected:
  virtual trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  const Settings m_settings;
  Client m_client;
  boost::unordered_map<size_t, boost::shared_ptr<FixProtocol::Security>>
      m_securities;
};
}
}
}