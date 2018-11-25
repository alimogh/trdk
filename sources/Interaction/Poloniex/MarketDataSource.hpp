//
//    Created: 2018/11/14 15:41
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
namespace Poloniex {

class MarketDataSource : public TradingLib::WebSocketMarketDataSource {
 public:
  typedef WebSocketMarketDataSource Base;

  explicit MarketDataSource(const Rest::App &,
                            Context &,
                            std::string instanceName,
                            std::string title,
                            const boost::property_tree::ptree &conf);
  MarketDataSource(MarketDataSource &&) = default;
  MarketDataSource(const MarketDataSource &) = delete;
  MarketDataSource &operator=(MarketDataSource &&) = delete;
  MarketDataSource &operator=(const MarketDataSource &) = delete;
  ~MarketDataSource() override = default;

  void SubscribeToSecurities() override;

  const boost::unordered_set<std::string> &GetSymbolListHint() const override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  std::unique_ptr<Connection> CreateConnection() const override;

  void HandleMessage(const boost::posix_time::ptime &,
                     const boost::property_tree::ptree &,
                     const Lib::TimeMeasurement::Milestones &) override;
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    SecuritySubscription &,
                    const Lib::TimeMeasurement::Milestones &);

  const Rest::Settings m_settings;

  const boost::unordered_map<std::string, Product> &m_products;
  const boost::unordered_set<std::string> m_symbolListHint;
  boost::unordered_map<ProductId, SecuritySubscription> m_securities;
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
