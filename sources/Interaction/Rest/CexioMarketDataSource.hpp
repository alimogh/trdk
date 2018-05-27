/*******************************************************************************
 *   Created: 2018/01/18 00:27:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "CexioUtil.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

//! CEX.IO market data source.
/** @sa https://cex.io/rest-api#public-api-calls
 */
class CexioMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

 private:
  struct Subscribtion {
    boost::shared_ptr<Rest::Security> security;
    boost::shared_ptr<Request> request;
    uintmax_t lastBookId;
  };

 public:
  explicit CexioMarketDataSource(const App &,
                                 Context &context,
                                 const std::string &instanceName,
                                 const boost::property_tree::ptree &);
  ~CexioMarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices();
  void UpdatePrices(const boost::property_tree::ptree &,
                    Subscribtion &,
                    const Lib::TimeMeasurement::Milestones &);

  boost::posix_time::ptime ParseTimeStamp(const boost::property_tree::ptree &);

  const Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  boost::unordered_map<std::string, CexioProduct> m_products;

  boost::mutex m_securitiesMutex;
  boost::unordered_map<CexioProduct *, Subscribtion> m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
