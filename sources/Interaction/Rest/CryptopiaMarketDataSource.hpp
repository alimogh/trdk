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

 public:
  explicit CryptopiaMarketDataSource(const App &,
                                     Context &context,
                                     const std::string &instanceName,
                                     const Lib::IniSectionRef &);
  virtual ~CryptopiaMarketDataSource() override;

 public:
  virtual void Connect(const Lib::IniSectionRef &conf) override;

  virtual void SubscribeToSecurities() override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void RequestActualPrices(Request &);
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

 private:
  const Settings m_settings;
  boost::unordered_map<std::string, CryptopiaProduct> m_products;
  boost::mutex m_securitiesLock;
  boost::unordered_map<CryptopiaProductId, boost::shared_ptr<Rest::Security>>
      m_securities;
  Poco::Net::HTTPSClientSession m_session;
  std::unique_ptr<PullingTask> m_pullingTask;
};
}
}
}
