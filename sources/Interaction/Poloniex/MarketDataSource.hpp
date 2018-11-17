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

#include "MarketDataConnection.hpp"
#include "Product.hpp"

namespace trdk {
namespace Interaction {
namespace Poloniex {

class MarketDataSource : public trdk::MarketDataSource {
  struct SecuritySubscription {
    boost::shared_ptr<Rest::Security> security;
    std::map<Price, std::pair<Level1TickValue, Level1TickValue>> bids;
    std::map<Price, std::pair<Level1TickValue, Level1TickValue>> asks;
  };

 public:
  typedef trdk::MarketDataSource Base;

  explicit MarketDataSource(const Rest::App &,
                            Context &,
                            std::string instanceName,
                            std::string title,
                            const boost::property_tree::ptree &conf);
  MarketDataSource(MarketDataSource &&) = delete;
  MarketDataSource(const MarketDataSource &) = delete;
  MarketDataSource &operator=(MarketDataSource &&) = delete;
  MarketDataSource &operator=(const MarketDataSource &) = delete;
  ~MarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

  const boost::unordered_set<std::string> &GetSymbolListHint() const override;

 protected:
  trdk::Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  void StartConnection(MarketDataConnection &);
  void ScheduleReconnect();

  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    const Lib::TimeMeasurement::Milestones &);
  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    SecuritySubscription &,
                    const Lib::TimeMeasurement::Milestones &);

  const Rest::Settings m_settings;

  const boost::unordered_map<std::string, Product> *m_products = nullptr;
  boost::unordered_set<std::string> m_symbolListHint;
  boost::unordered_map<ProductId, SecuritySubscription> m_securities;

  boost::mutex m_connectionMutex;
  bool m_isStarted = false;
  boost::shared_ptr<MarketDataConnection> m_connection;

  Timer::Scope m_timerScope;
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk