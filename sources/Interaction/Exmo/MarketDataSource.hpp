//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Product.hpp"

namespace trdk {
namespace Interaction {
namespace Exmo {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  typedef trdk::MarketDataSource Base;

  explicit MarketDataSource(const Rest::App &,
                            Context &context,
                            const std::string &instanceName,
                            const boost::property_tree::ptree &conf);
  MarketDataSource(MarketDataSource &&) = default;
  MarketDataSource(const MarketDataSource &) = delete;
  MarketDataSource &operator=(const MarketDataSource &) = delete;
  MarketDataSource &operator=(MarketDataSource &&) = default;
  ~MarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void UpdatePrices(Rest::Request &);
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

  const Rest::Settings m_settings;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  boost::unordered_map<std::string, Product> m_products;
  boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
      m_securities;

  std::unique_ptr<Rest::PollingTask> m_pollingTask;
};

}  // namespace Exmo
}  // namespace Interaction
}  // namespace trdk
