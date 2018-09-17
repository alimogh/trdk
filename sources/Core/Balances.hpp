/*******************************************************************************
 *   Created: 2017/11/29 15:35:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

class Balances {
 public:
  Balances() = default;
  Balances(Balances &&) = default;
  Balances(const Balances &) = delete;
  Balances &operator=(Balances &&) = delete;
  Balances &operator=(const Balances &) = delete;
  virtual ~Balances() = default;

  virtual Volume GetAvailableToTrade(const std::string &symbol) const = 0;
  virtual void ReduceAvailableToTradeByOrder(const Security &,
                                             const Qty &,
                                             const Price &,
                                             const OrderSide &,
                                             const TradingSystem &) = 0;
  virtual void ForEach(
      const boost::function<void(const std::string &symbol,
                                 const Volume &available,
                                 const Volume &locked)> &) const = 0;
};
}  // namespace trdk
