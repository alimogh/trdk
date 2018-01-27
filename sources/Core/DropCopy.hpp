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

#include "Api.h"
#include "Pnl.hpp"
#include "TradingSystem.hpp"

namespace trdk {

class TRDK_CORE_API DropCopy {
 public:
  class TRDK_CORE_API Exception : public trdk::Lib::Exception {
   public:
    explicit Exception(const char *what) throw();
  };

 public:
  static const trdk::DropCopyStrategyInstanceId nStrategyInstanceId;
  static const trdk::DropCopyDataSourceInstanceId nDataSourceInstanceId;

 public:
  DropCopy();
  virtual ~DropCopy();

 public:
  //! Tries to flush buffered Drop Copy data.
  /** The method doesn't guarantee to store all records, it just initiates
   * new send attempt. Synchronous. Can be interrupted from another
   * thread.
   */
  virtual void Flush() = 0;

  //! Dumps all buffer data and removes it from buffer.
  virtual void Dump() = 0;

 public:
  virtual trdk::DropCopyStrategyInstanceId RegisterStrategyInstance(
      const trdk::Strategy &) = 0;
  virtual trdk::DropCopyStrategyInstanceId ContinueStrategyInstance(
      const trdk::Strategy &, const boost::posix_time::ptime &) = 0;
  virtual trdk::DropCopyDataSourceInstanceId RegisterDataSourceInstance(
      const trdk::Strategy &,
      const boost::uuids::uuid &type,
      const boost::uuids::uuid &id) = 0;

  virtual void CopySubmittedOrder(const trdk::OrderId &,
                                  const boost::posix_time::ptime &,
                                  const trdk::Security &,
                                  const trdk::Lib::Currency &,
                                  const trdk::TradingSystem &,
                                  const trdk::OrderSide &,
                                  const trdk::Qty &,
                                  const boost::optional<trdk::Price> &,
                                  const trdk::TimeInForce &) = 0;
  virtual void CopySubmittedOrder(const trdk::OrderId &,
                                  const boost::posix_time::ptime &,
                                  const trdk::Position &,
                                  const trdk::OrderSide &,
                                  const trdk::Qty &,
                                  const boost::optional<trdk::Price> &,
                                  const trdk::TimeInForce &) = 0;
  virtual void CopyOrderStatus(const trdk::OrderId &,
                               const trdk::TradingSystem &,
                               const boost::posix_time::ptime &,
                               const trdk::OrderStatus &,
                               const trdk::Qty &remainingQty) = 0;
  virtual void CopyOrder(const trdk::OrderId &,
                         const trdk::TradingSystem &,
                         const std::string &symbol,
                         const trdk::OrderStatus &,
                         const trdk::Qty &qty,
                         const trdk::Qty &remainingQty,
                         const boost::optional<trdk::Price> &,
                         const trdk::OrderSide &,
                         const trdk::TimeInForce &,
                         const boost::posix_time::ptime &openTime,
                         const boost::posix_time::ptime &updateTime) = 0;

  virtual void CopyTrade(
      const boost::posix_time::ptime &,
      const boost::optional<std::string> &tradingSystemTradeId,
      const trdk::OrderId &,
      const trdk::TradingSystem &,
      const trdk::Price &,
      const trdk::Qty &) = 0;

  virtual void CopyOperationStart(const boost::uuids::uuid &,
                                  const boost::posix_time::ptime &,
                                  const trdk::Strategy &) = 0;
  virtual void CopyOperationUpdate(const boost::uuids::uuid &,
                                   const trdk::Pnl::Data &) = 0;
  virtual void CopyOperationEnd(const boost::uuids::uuid &,
                                const boost::posix_time::ptime &,
                                std::unique_ptr<trdk::Pnl> &&) = 0;

  virtual void CopyBook(const trdk::Security &, const trdk::PriceBook &) = 0;

  virtual void CopyBar(const trdk::DropCopyDataSourceInstanceId &,
                       size_t index,
                       const boost::posix_time::ptime &,
                       const trdk::Price &open,
                       const trdk::Price &high,
                       const trdk::Price &low,
                       const trdk::Price &close) = 0;

  virtual void CopyAbstractData(const trdk::DropCopyDataSourceInstanceId &,
                                size_t index,
                                const boost::posix_time::ptime &,
                                const trdk::Lib::Double &value) = 0;

  virtual void CopyLevel1(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Level1TickValue &) = 0;
  virtual void CopyLevel1(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &) = 0;
  virtual void CopyLevel1(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &) = 0;
  virtual void CopyLevel1(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &,
                          const trdk::Level1TickValue &) = 0;
  virtual void CopyLevel1(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const std::vector<trdk::Level1TickValue> &) = 0;

  virtual void CopyBalance(const trdk::TradingSystem &,
                           const std::string &symbol,
                           const trdk::Volume &available,
                           const trdk::Volume &locked) = 0;
};
}  // namespace trdk
