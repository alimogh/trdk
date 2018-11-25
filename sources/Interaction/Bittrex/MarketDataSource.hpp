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

#include "Util.hpp"

namespace trdk {
namespace Interaction {
namespace Bittrex {

class MarketDataSource : public TradingLib::WebSocketMarketDataSource {
 public:
  typedef WebSocketMarketDataSource Base;

  explicit MarketDataSource(const Rest::App &,
                            Context &context,
                            std::string instanceName,
                            std::string title,
                            const boost::property_tree::ptree &);
  MarketDataSource(MarketDataSource &&) = default;
  MarketDataSource(const MarketDataSource &) = delete;
  const MarketDataSource &operator=(MarketDataSource &&) = delete;
  const MarketDataSource &operator=(const MarketDataSource &) = delete;
  ~MarketDataSource() override = default;

  void Connect() override;

  const boost::unordered_set<std::string> &GetSymbolListHint() const override;

 protected:
  Security &CreateNewSecurityObject(const Lib::Symbol &) override;

 private:
  std::unique_ptr<Connection> CreateConnection() const override;

  void HandleMessage(const boost::posix_time::ptime &,
                     const boost::property_tree::ptree &,
                     const Lib::TimeMeasurement::Milestones &) override;

  void UpdatePrices(const boost::posix_time::ptime &,
                    const boost::property_tree::ptree &,
                    Rest::Security &,
                    const Lib::TimeMeasurement::Milestones &);

  const Rest::Settings m_settings;
  const boost::unordered_map<std::string, Product> *m_products = nullptr;
  boost::unordered_set<std::string> m_symbolListHint;
  boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
      m_securities;
};

}  // namespace Bittrex
}  // namespace Interaction
}  // namespace trdk
