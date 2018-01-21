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

class Balances : private boost::noncopyable {
 public:
  virtual ~Balances() = default;

 public:
  virtual trdk::Volume FindAvailableToTrade(
      const std::string &symbol) const = 0;
  virtual void ReduceAvailableToTradeByOrder(const trdk::Security &,
                                             const trdk::Qty &,
                                             const trdk::Price &,
                                             const trdk::OrderSide &,
                                             const trdk::TradingSystem &) = 0;
  virtual void ForEach(
      const boost::function<void(const std::string &symbol,
                                 const trdk::Volume &available,
                                 const trdk::Volume &locked)> &) const = 0;
};
}
