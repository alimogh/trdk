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

 public:
  explicit Context(
      trdk::Context::Log &,
      trdk::Context::TradingLog &,
      const trdk::Settings &,
      const trdk::Lib::Ini &,
      const boost::unordered_map<std::string, std::string> &params);
  virtual ~Context() override;

 public:
  void Start(
      const trdk::Lib::Ini &,
      const boost::function<void(const std::string &)> &startProgressCallback,
      const boost::function<bool(const std::string &)> &startErrorCallback,
      trdk::DropCopy * = nullptr);
  void Stop(const trdk::StopMode &);

  virtual void Add(const trdk::Lib::Ini &) override;
  void Update(const trdk::Lib::Ini &);

  virtual std::unique_ptr<DispatchingLock> SyncDispatching() const override;

 public:
  virtual RiskControl &GetRiskControl(const trdk::TradingMode &) override;
  virtual const RiskControl &GetRiskControl(
      const trdk::TradingMode &) const override;

  virtual const trdk::Lib::ExpirationCalendar &GetExpirationCalendar()
      const override;
  virtual bool HasExpirationCalendar() const override;

  virtual size_t GetNumberOfMarketDataSources() const override;
  virtual const trdk::MarketDataSource &GetMarketDataSource(
      size_t index) const override;
  virtual trdk::MarketDataSource &GetMarketDataSource(size_t index) override;
  virtual void ForEachMarketDataSource(
      const boost::function<void(const trdk::MarketDataSource &)> &)
      const override;
  virtual void ForEachMarketDataSource(
      const boost::function<void(trdk::MarketDataSource &)> &) override;

  virtual size_t GetNumberOfTradingSystems() const override;
  virtual const trdk::TradingSystem &GetTradingSystem(
      size_t index, const TradingMode &) const override;
  virtual trdk::TradingSystem &GetTradingSystem(size_t index,
                                                const TradingMode &) override;

  virtual trdk::Strategy &GetSrategy(const boost::uuids::uuid &id) override;
  virtual const trdk::Strategy &GetSrategy(
      const boost::uuids::uuid &id) const override;

  virtual void CloseSrategiesPositions() override;

 protected:
  virtual DropCopy *GetDropCopy() const override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
