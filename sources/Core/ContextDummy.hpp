/**************************************************************************
 *   Created: 2016/09/12 21:18:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Context.hpp"

namespace trdk {
namespace Tests {
namespace Dummies {

class Context : public trdk::Context {
 public:
  Context();
  ~Context() override = default;

  static Context& GetInstance();

  std::unique_ptr<DispatchingLock> SyncDispatching() const override;

  RiskControl& GetRiskControl(const TradingMode&) override;
  const RiskControl& GetRiskControl(const TradingMode& mode) const override;

  const Lib::ExpirationCalendar& GetExpirationCalendar() const override;
  bool HasExpirationCalendar() const override;

  boost::unordered_set<std::string> GetSymbolListHint() const override;

  size_t GetNumberOfMarketDataSources() const override;
  const MarketDataSource& GetMarketDataSource(size_t) const override;
  MarketDataSource& GetMarketDataSource(size_t) override;

  void ForEachMarketDataSource(
      const boost::function<void(const trdk::MarketDataSource&)>&)
      const override;
  void ForEachMarketDataSource(
      const boost::function<void(trdk::MarketDataSource&)>&) override;

  size_t GetNumberOfTradingSystems() const override;
  const TradingSystem& GetTradingSystem(size_t,
                                        const TradingMode&) const override;
  TradingSystem& GetTradingSystem(size_t, const TradingMode&) override;

  Strategy& GetSrategy(const boost::uuids::uuid&) override;
  const Strategy& GetSrategy(const boost::uuids::uuid&) const override;

  void CloseSrategiesPositions() override;

  void Add(const boost::property_tree::ptree&) override;

 protected:
  DropCopy* GetDropCopy() const override;
};
}  // namespace Dummies
}  // namespace Tests
}  // namespace trdk
