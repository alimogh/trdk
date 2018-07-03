/**************************************************************************
 *   Created: 2015/07/12 19:05:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Pnl.hpp"

namespace trdk {

class DropCopy {
 public:
  class Exception : public Lib::Exception {
   public:
    explicit Exception(const char* what) noexcept : Lib::Exception(what) {}
  };

  DropCopy() = default;
  DropCopy(DropCopy&&) = default;
  DropCopy(const DropCopy&) = delete;
  DropCopy& operator=(DropCopy&&) = default;
  DropCopy& operator=(const DropCopy&) = delete;
  virtual ~DropCopy() = default;

  //! Tries to flush buffered Drop Copy data.
  /**
   * The method doesn't guarantee to store all records, it just initiates
   * new send attempt. Synchronous. Can be interrupted from another thread.
   */
  virtual void Flush() = 0;

  //! Dumps all buffer data and removes it from buffer.
  virtual void Dump() = 0;

  virtual void CopySubmittedOrder(const OrderId&,
                                  const boost::posix_time::ptime&,
                                  const Security&,
                                  const Lib::Currency&,
                                  const TradingSystem&,
                                  const OrderSide&,
                                  const Qty&,
                                  const boost::optional<Price>&,
                                  const TimeInForce&) = 0;
  virtual void CopySubmittedOrder(const OrderId&,
                                  const boost::posix_time::ptime&,
                                  const Position&,
                                  const OrderSide&,
                                  const Qty&,
                                  const boost::optional<Price>&,
                                  const TimeInForce&) = 0;
  virtual void CopyOrderSubmitError(const boost::posix_time::ptime&,
                                    const Security&,
                                    const Lib::Currency&,
                                    const TradingSystem&,
                                    const OrderSide&,
                                    const Qty&,
                                    const boost::optional<Price>&,
                                    const TimeInForce&,
                                    const std::string& error) = 0;
  virtual void CopyOrderSubmitError(const boost::posix_time::ptime&,
                                    const Position&,
                                    const OrderSide&,
                                    const Qty&,
                                    const boost::optional<Price>&,
                                    const TimeInForce&,
                                    const std::string& error) = 0;
  virtual void CopyOrderStatus(const OrderId&,
                               const TradingSystem&,
                               const boost::posix_time::ptime&,
                               const OrderStatus&,
                               const Qty& remainingQty) = 0;

  virtual void CopyTrade(
      const boost::posix_time::ptime&,
      const boost::optional<std::string>& tradingSystemTradeId,
      const OrderId&,
      const TradingSystem&,
      const Price&,
      const Qty&) = 0;

  virtual void CopyOperationStart(const boost::uuids::uuid&,
                                  const boost::posix_time::ptime&,
                                  const Strategy&) = 0;
  virtual void CopyOperationUpdate(const boost::uuids::uuid&,
                                   const Pnl::Data&) = 0;
  virtual void CopyOperationEnd(const boost::uuids::uuid&,
                                const boost::posix_time::ptime&,
                                std::unique_ptr<Pnl>&&) = 0;

  virtual void CopyBook(const Security&, const PriceBook&) = 0;

  virtual void CopyBar(const Security&, const Bar&) = 0;

  virtual void CopyLevel1(const Security&,
                          const boost::posix_time::ptime&,
                          const Level1TickValue&) = 0;
  virtual void CopyLevel1(const Security&,
                          const boost::posix_time::ptime&,
                          const Level1TickValue&,
                          const Level1TickValue&) = 0;
  virtual void CopyLevel1(const Security&,
                          const boost::posix_time::ptime&,
                          const Level1TickValue&,
                          const Level1TickValue&,
                          const Level1TickValue&) = 0;
  virtual void CopyLevel1(const Security&,
                          const boost::posix_time::ptime&,
                          const Level1TickValue&,
                          const Level1TickValue&,
                          const Level1TickValue&,
                          const Level1TickValue&) = 0;
  virtual void CopyLevel1(const Security&,
                          const boost::posix_time::ptime&,
                          const std::vector<Level1TickValue>&) = 0;

  virtual void CopyBalance(const TradingSystem&,
                           const std::string& symbol,
                           const Volume& available,
                           const Volume& locked) = 0;
};

}  // namespace trdk
