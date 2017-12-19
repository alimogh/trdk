/*******************************************************************************
 *   Created: 2017/12/12 16:54:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "LivecoinRequest.hpp"
#include "LivecoinUtil.hpp"
#include "Settings.hpp"
#include "Util.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

//! Livecoin market data source.
/** @sa https://www.livecoin.net/api/public
  */
class LivecoinMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

 public:
  explicit LivecoinMarketDataSource(const App &,
                                    Context &context,
                                    const std::string &instanceName,
                                    const Lib::IniSectionRef &);
  virtual ~LivecoinMarketDataSource() override;

 public:
  virtual void Connect(const Lib::IniSectionRef &conf) override;

  virtual void SubscribeToSecurities() override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices();
  void UpdatePrices(const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

 private:
  const Settings m_settings;
  boost::unordered_map<std::string, LivecoinProduct> m_products;

  boost::mutex m_securitiesLock;
  boost::unordered_map<LivecoinProductId, boost::shared_ptr<Rest::Security>>
      m_securities;
  LivecoinPublicRequest m_allOrderBooksRequest;

  Poco::Net::HTTPSClientSession m_session;

  std::unique_ptr<PullingTask> m_pullingTask;
};
}
}
}
