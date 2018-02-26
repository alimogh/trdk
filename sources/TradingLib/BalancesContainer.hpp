/*******************************************************************************
 *   Created: 2017/11/29 15:41:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Core/Balances.hpp"

namespace trdk {
namespace TradingLib {

class BalancesContainer : public trdk::Balances {
 public:
  explicit BalancesContainer(const trdk::TradingSystem &,
                             trdk::ModuleEventsLog &,
                             trdk::ModuleTradingLog &);
  virtual ~BalancesContainer() override;

 public:
  virtual trdk::Volume FindAvailableToTrade(
      const std::string &symbol) const override;

 public:
  virtual void ForEach(
      const boost::function<void(
          const std::string &, const trdk::Volume &, const trdk::Volume &)> &)
      const override;

  void Set(const std::string &symbol,
           trdk::Volume &&available,
           trdk::Volume &&locked);
  void SetAvailableToTrade(const std::string &symbol, trdk::Volume &&);
  void SetLocked(const std::string &symbol, trdk::Volume &&);

  virtual void ReduceAvailableToTradeByOrder(
      const trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderSide &,
      const trdk::TradingSystem &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace TradingLib
}  // namespace trdk
