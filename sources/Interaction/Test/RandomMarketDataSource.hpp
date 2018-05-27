/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Security.hpp"

namespace trdk {
namespace Interaction {
namespace Test {

class RandomMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

  explicit RandomMarketDataSource(Context &context,
                                  const std::string &instanceName,
                                  const boost::property_tree::ptree &);
  ~RandomMarketDataSource() override;

  void Connect() override;
  void SubscribeToSecurities() override {}

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void NotificationThread();

  boost::thread_group m_threads;
  boost::atomic_bool m_stopFlag;

  std::vector<boost::shared_ptr<Security>> m_securityList;
};
}  // namespace Test
}  // namespace Interaction
}  // namespace trdk
