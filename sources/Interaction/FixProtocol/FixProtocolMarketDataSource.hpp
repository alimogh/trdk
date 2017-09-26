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

 public:
  virtual void Connect(const trdk::Lib::IniSectionRef &) override;
  virtual void SubscribeToSecurities() override;
  void ResubscribeToSecurities();

 protected:
  virtual Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  const Settings m_settings;
  Client m_client;
  std::vector<boost::shared_ptr<Security>> m_securities;
};
}
}
}