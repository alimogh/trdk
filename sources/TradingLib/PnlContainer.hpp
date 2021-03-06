/*******************************************************************************
 *   Created: 2018/01/23 11:21:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/PnlContainer.hpp"

namespace trdk {
namespace TradingLib {

class PnlOneSymbolContainer : public PnlContainer {
 public:
  PnlOneSymbolContainer();
  PnlOneSymbolContainer(PnlOneSymbolContainer &&) = default;
  PnlOneSymbolContainer(const PnlOneSymbolContainer &) = delete;
  PnlOneSymbolContainer &operator=(PnlOneSymbolContainer &&) = delete;
  PnlOneSymbolContainer &operator=(const PnlOneSymbolContainer &) = delete;
  ~PnlOneSymbolContainer() override;

  void UpdateFinancialResult(const Security &,
                             const OrderSide &,
                             const Qty &,
                             const Price &) override;
  void UpdateFinancialResult(const Security &,
                             const OrderSide &,
                             const Qty &,
                             const Price &,
                             const Volume &commission) override;

  void AddCommission(const Security &, const Volume &) override;

  Result GetResult() const override;
  const Data &GetData() const override;

 private:
  virtual Result CalcResult(size_t numberOfProfits,
                            size_t numberOfLosses) const;

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace TradingLib
}  // namespace trdk
