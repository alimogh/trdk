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

class BalancesContainer : public Balances {
 public:
  explicit BalancesContainer(const TradingSystem &,
                             ModuleEventsLog &,
                             ModuleTradingLog &);
  ~BalancesContainer() override;

  Volume GetAvailableToTrade(const std::string &symbol) const override;

  void ForEach(const boost::function<
               void(const std::string &, const Volume &, const Volume &)> &)
      const override;

  void Set(const std::string &symbol, Volume available, Volume locked);
  void SetAvailableToTrade(const std::string &symbol, Volume);
  void SetLocked(const std::string &symbol, Volume);

  void ReduceAvailableToTradeByOrder(const Security &,
                                     const Qty &,
                                     const Price &,
                                     const OrderSide &,
                                     const TradingSystem &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace TradingLib
}  // namespace trdk
