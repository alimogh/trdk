/*******************************************************************************
 *   Created: 2017/12/04 16:55:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "CryptopiaUtil.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

//! Cryptopia market data source.
/** @sa https://www.cryptopia.co.nz/Forum/Thread/255
 */
class CryptopiaMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

  explicit CryptopiaMarketDataSource(const App &,
                                     Context &context,
                                     std::string instanceName,
                                     std::string title,
                                     const boost::property_tree::ptree &);
  ~CryptopiaMarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices(Request &);
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    const CryptopiaProduct &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

  const Settings m_settings;
  CryptopiaProductList m_products;

  boost::mutex m_securitiesLock;
  boost::unordered_map<CryptopiaProductId,
                       std::pair<CryptopiaProductList::const_iterator,
                                 boost::shared_ptr<Rest::Security>>>
      m_securities;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  std::unique_ptr<PollingTask> m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
