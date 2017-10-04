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
  virtual ~Context() override = default;

  static Context &GetInstance();

 public:
  virtual std::unique_ptr<DispatchingLock> SyncDispatching() const override;

  virtual RiskControl &GetRiskControl(const trdk::TradingMode &) override;
  virtual const RiskControl &GetRiskControl(
      const trdk::TradingMode &mode) const override;

  virtual const trdk::Lib::ExpirationCalendar &GetExpirationCalendar()
      const override;
  virtual bool HasExpirationCalendar() const override;

  virtual size_t GetNumberOfMarketDataSources() const override;
  virtual const trdk::MarketDataSource &GetMarketDataSource(
      size_t) const override;
  virtual trdk::MarketDataSource &GetMarketDataSource(size_t) override;

  virtual void ForEachMarketDataSource(
      const boost::function<void(const trdk::MarketDataSource &)> &)
      const override;
  virtual void ForEachMarketDataSource(
      const boost::function<void(trdk::MarketDataSource &)> &) override;

  virtual size_t GetNumberOfTradingSystems() const override;
  virtual const trdk::TradingSystem &GetTradingSystem(
      size_t, const trdk::TradingMode &) const override;
  virtual trdk::TradingSystem &GetTradingSystem(
      size_t, const trdk::TradingMode &) override;

 protected:
  virtual DropCopy *GetDropCopy() const override;
};
}
}
}