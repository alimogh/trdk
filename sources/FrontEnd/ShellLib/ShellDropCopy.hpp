/*******************************************************************************
 *   Created: 2017/09/29 09:41:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/DropCopy.hpp"
#include "ShellApi.h"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class TRDK_FRONTEND_SHELL_LIB_API DropCopy : public QObject,
                                             public trdk::DropCopy {
  Q_OBJECT

 public:
  explicit DropCopy(QObject *parent);
  virtual ~DropCopy() override = default;

 signals:
  void PriceUpdate(const Security &);

 public:
  //! Tries to flush buffered Drop Copy data.
  /** The method doesn't guarantee to store all records, it just initiates
    * new send attempt. Synchronous. Can be interrupted from another
    * thread.
    */
  virtual void Flush() override;

  //! Dumps all buffer data and removes it from buffer.
  virtual void Dump() override;

 public:
  virtual trdk::DropCopyStrategyInstanceId RegisterStrategyInstance(
      const trdk::Strategy &) override;
  virtual trdk::DropCopyStrategyInstanceId ContinueStrategyInstance(
      const trdk::Strategy &, const boost::posix_time::ptime &) override;
  virtual trdk::DropCopyDataSourceInstanceId RegisterDataSourceInstance(
      const trdk::Strategy &,
      const boost::uuids::uuid &type,
      const boost::uuids::uuid &id) override;

  virtual void CopyOrder(const boost::uuids::uuid &id,
                         const std::string *tradingSystemId,
                         const boost::posix_time::ptime &orderTime,
                         const boost::posix_time::ptime *executionTime,
                         const trdk::OrderStatus &,
                         const boost::uuids::uuid &operationId,
                         const int64_t *subOperationId,
                         const trdk::Security &,
                         const trdk::TradingSystem &,
                         const trdk::OrderSide &,
                         const trdk::Qty &qty,
                         const trdk::Price *price,
                         const trdk::TimeInForce *,
                         const trdk::Lib::Currency &,
                         const trdk::Qty *minQty,
                         const trdk::Qty &executedQty,
                         const trdk::Price *bestBidPrice,
                         const trdk::Qty *bestBidQty,
                         const trdk::Price *bestAskPrice,
                         const trdk::Qty *bestAskQty) override;

  virtual void CopyTrade(const boost::posix_time::ptime &,
                         const std::string &tradingSystemTradeId,
                         const boost::uuids::uuid &orderId,
                         const trdk::Price &price,
                         const trdk::Qty &qty,
                         const trdk::Price &bestBidPrice,
                         const trdk::Qty &bestBidQty,
                         const trdk::Price &bestAskPrice,
                         const trdk::Qty &bestAskQty) override;

  virtual void ReportOperationStart(const trdk::Strategy &,
                                    const boost::uuids::uuid &id,
                                    const boost::posix_time::ptime &) override;
  virtual void ReportOperationEnd(const boost::uuids::uuid &id,
                                  const boost::posix_time::ptime &,
                                  const trdk::CloseReason &,
                                  const trdk::OperationResult &,
                                  const trdk::Volume &pnl,
                                  trdk::FinancialResult &&) override;

  virtual void CopyBook(const trdk::Security &,
                        const trdk::PriceBook &) override;

  virtual void CopyBar(const trdk::DropCopyDataSourceInstanceId &,
                       size_t index,
                       const boost::posix_time::ptime &,
                       const trdk::Price &open,
                       const trdk::Price &high,
                       const trdk::Price &low,
                       const trdk::Price &close) override;

  virtual void CopyAbstractData(const trdk::DropCopyDataSourceInstanceId &,
                                size_t index,
                                const boost::posix_time::ptime &,
                                const trdk::Lib::Double &value) override;

  virtual void CopyLevel1Tick(const trdk::Security &,
                              const boost::posix_time::ptime &,
                              const trdk::Level1TickValue &) override;
  virtual void CopyLevel1Tick(const trdk::Security &,
                              const boost::posix_time::ptime &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &) override;
  virtual void CopyLevel1Tick(const trdk::Security &,
                              const boost::posix_time::ptime &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &) override;
  virtual void CopyLevel1Tick(const trdk::Security &,
                              const boost::posix_time::ptime &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &,
                              const trdk::Level1TickValue &) override;
  virtual void CopyLevel1Tick(
      const trdk::Security &,
      const boost::posix_time::ptime &,
      const std::vector<trdk::Level1TickValue> &) override;

 private:
  void EmitLevel1Tick(const Security &, const Level1TickValue &);
};
}
}
}