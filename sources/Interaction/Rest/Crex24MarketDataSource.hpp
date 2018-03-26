/*******************************************************************************
 *   Created: 2018/02/11 17:58:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Crex24Util.hpp"
#include "ExcambiorexUtil.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class Crex24MarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

 public:
  explicit Crex24MarketDataSource(const App &,
                                  Context &context,
                                  const std::string &instanceName,
                                  const Lib::IniSectionRef &);
  virtual ~Crex24MarketDataSource() override;

 public:
  virtual void Connect(const Lib::IniSectionRef &conf) override;

  virtual void SubscribeToSecurities() override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices(const std::vector<std::pair<boost::shared_ptr<Security>,
                                                boost::shared_ptr<Request>>> &);
  void UpdatePrices(Security &, Request &);

 private:
  const Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  boost::unordered_map<std::string,
                       std::pair<Crex24Product, boost::shared_ptr<Security>>>
      m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
