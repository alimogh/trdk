/*******************************************************************************
 *   Created: 2017/11/16 12:04:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Request.hpp"
#include "Util.hpp"

namespace trdk {
namespace Interaction {
namespace Bittrex {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  typedef trdk::MarketDataSource Base;

  explicit MarketDataSource(const Rest::App &,
                            Context &context,
                            std::string instanceName,
                            std::string title,
                            const boost::property_tree::ptree &);
  ~MarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

  const boost::unordered_set<std::string> &GetSymbolListHint() const override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices();
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

  const Rest::Settings m_settings;
  boost::unordered_map<std::string, Product> m_products;
  boost::unordered_set<std::string> m_symbolListHint;
  boost::mutex m_securitiesLock;
  std::vector<std::pair<boost::shared_ptr<Rest::Security>,
                        std::unique_ptr<PublicRequest>>>
      m_securities;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;
  std::unique_ptr<Rest::PollingTask> m_pollingTask;
};

}  // namespace Bittrex
}  // namespace Interaction
}  // namespace trdk
