/**************************************************************************
 *   Created: 2016/10/15 14:11:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace trdk {
namespace Interaction {
namespace Test {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  typedef trdk::MarketDataSource Base;

  MarketDataSource(Context &context,
                   std::string instanceName,
                   std::string title,
                   const boost::property_tree::ptree &);
  ~MarketDataSource() override = default;

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  virtual void Run() = 0;

  //! Should be called from each inherited destructor.
  void Stop();

  bool IsStopped() const { return m_stopFlag; }

 private:
  boost::thread_group m_threads;
  boost::atomic_bool m_stopFlag;
};
}  // namespace Test
}  // namespace Interaction
}  // namespace trdk
