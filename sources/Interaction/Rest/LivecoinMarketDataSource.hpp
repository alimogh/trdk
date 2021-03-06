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

namespace trdk {
namespace Interaction {
namespace Rest {

//! Livecoin market data source.
/** @sa https://www.livecoin.net/api/public
 */
class LivecoinMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

  explicit LivecoinMarketDataSource(const App &,
                                    Context &context,
                                    std::string instanceName,
                                    std::string title,
                                    const boost::property_tree::ptree &);
  ~LivecoinMarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

  const boost::unordered_set<std::string> &GetSymbolListHint() const override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices();
  void UpdatePrices(const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

  const Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  boost::unordered_map<std::string, LivecoinProduct> m_products;
  boost::unordered_set<std::string> m_symbolListHint;

  boost::mutex m_securitiesMutex;
  boost::unordered_map<LivecoinProductId, boost::shared_ptr<Security>>
      m_securities;
  LivecoinPublicRequest m_allOrderBooksRequest;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  std::unique_ptr<PollingTask> m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
