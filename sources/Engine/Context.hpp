/**************************************************************************
 *   Created: 2013/01/31 00:48:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"
#include "Core/DropCopy.hpp"
#include "Api.h"

namespace trdk {
namespace Engine {

class TRDK_ENGINE_API Context : public trdk::Context {
 public:
  typedef trdk::Context Base;

  explicit Context(Log &, TradingLog &, Settings &&);
  Context(Context &&) noexcept;
  Context(const Context &) = delete;
  Context &operator=(Context &&) = delete;
  Context &operator=(const Context &) = delete;
  ~Context() override;

  void Start(
      const boost::function<void(const std::string &)> &startProgressCallback,
      const boost::function<bool(const std::string &)> &startErrorCallback,
      DropCopy * = nullptr);
  void Stop(const StopMode &);

  std::unique_ptr<AddingTransaction> StartAdding() override;

  std::unique_ptr<DispatchingLock> SyncDispatching() const override;

  RiskControl &GetRiskControl(const TradingMode &) override;
  const RiskControl &GetRiskControl(const TradingMode &) const override;

  const Lib::ExpirationCalendar &GetExpirationCalendar() const override;
  bool HasExpirationCalendar() const override;

  boost::unordered_set<std::string> GetSymbolListHint() const override;

  size_t GetNumberOfMarketDataSources() const override;
  const MarketDataSource &GetMarketDataSource(size_t index) const override;
  MarketDataSource &GetMarketDataSource(size_t index) override;
  void ForEachMarketDataSource(
      const boost::function<void(const MarketDataSource &)> &) const override;
  void ForEachMarketDataSource(
      const boost::function<void(MarketDataSource &)> &) override;

  size_t GetNumberOfTradingSystems() const override;
  const TradingSystem &GetTradingSystem(size_t index,
                                        const TradingMode &) const override;
  TradingSystem &GetTradingSystem(size_t index, const TradingMode &) override;

  Strategy &GetStrategy(const boost::uuids::uuid &id) override;
  const Strategy &GetStrategy(const boost::uuids::uuid &id) const override;

  void CloseStrategiesPositions() override;

 protected:
  DropCopy *GetDropCopy() const override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace Engine
}  // namespace trdk
