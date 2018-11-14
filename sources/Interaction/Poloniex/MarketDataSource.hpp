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

class MarketDataSource : public trdk::MarketDataSource {
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

  const Rest::Settings m_settings;

  boost::unordered_map<std::string, Product> m_products;
  boost::unordered_set<std::string> m_symbolListHint;
  boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
      m_securities;

  boost::mutex m_connectionMutex;
  bool m_isStarted = false;

  Timer::Scope m_timerScope;
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk