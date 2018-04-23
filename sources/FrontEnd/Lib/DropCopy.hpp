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
#include "Api.h"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API DropCopy : public QObject, public trdk::DropCopy {
  Q_OBJECT

 public:
  explicit DropCopy(QObject* parent);
  ~DropCopy() override = default;

 signals:
  void PriceUpdate(const Security*);

  void FreeOrderSubmit(const trdk::OrderId &,
                       const boost::posix_time::ptime&,
                       const trdk::Security *,
                       const trdk::Lib::Currency &,
                       const trdk::TradingSystem *,
                       const trdk::OrderSide &,
                       const trdk::Qty &,
                       const boost::optional<trdk::Price> &,
                       const trdk::TimeInForce &);
  void OperationOrderSubmit(const boost::uuids::uuid& operationId,
                            int64_t subOperationId,
                            const trdk::OrderId &,
                            const boost::posix_time::ptime&,
                            const trdk::Security *,
                            const trdk::Lib::Currency &,
                            const trdk::TradingSystem *,
                            const trdk::OrderSide &,
                            const trdk::Qty &,
                            const boost::optional<trdk::Price> &,
                            const trdk::TimeInForce &);
  void FreeOrderSubmitError(const boost::posix_time::ptime&,
                            const trdk::Security *,
                            const trdk::Lib::Currency &,
                            const trdk::TradingSystem *,
                            const trdk::OrderSide &,
                            const trdk::Qty &,
                            const boost::optional<trdk::Price> &,
                            const trdk::TimeInForce &,
                            const QString& error);
  void OperationOrderSubmitError(const boost::uuids::uuid& operationId,
                                 int64_t subOperationId,
                                 const boost::posix_time::ptime&,
                                 const trdk::Security *,
                                 const trdk::Lib::Currency &,
                                 const trdk::TradingSystem *,
                                 const trdk::OrderSide &,
                                 const trdk::Qty &,
                                 const boost::optional<trdk::Price> &,
                                 const trdk::TimeInForce &,
                                 const QString& error);
  void OrderUpdate(const trdk::OrderId &,
                   const trdk::TradingSystem *,
                   const boost::posix_time::ptime&,
                   const trdk::OrderStatus &,
                   const trdk::Qty &remainingQty);
  void Order(const trdk::OrderId &,
             const trdk::TradingSystem *,
             const std::string& symbol,
             const trdk::OrderStatus &,
             const trdk::Qty &qty,
             const trdk::Qty &remainingQty,
             const boost::optional<trdk::Price> &,
             const trdk::OrderSide &,
             const trdk::TimeInForce &,
             const boost::posix_time::ptime& openTime,
             const boost::posix_time::ptime& updateTime);

  void BalanceUpdate(const trdk::TradingSystem *,
                     const std::string& symbol,
                     const trdk::Volume &available,
                     const trdk::Volume &locked);

  void OperationStart(const boost::uuids::uuid&,
                      const boost::posix_time::ptime&,
                      const trdk::Strategy *);
  void OperationUpdate(const boost::uuids::uuid &, const trdk::Pnl::Data &);
  void OperationEnd(const boost::uuids::uuid&,
                    const boost::posix_time::ptime&,
                    const boost::shared_ptr<const trdk::Pnl> &);

 public:
  //! Tries to flush buffered Drop Copy data.
  /**
   * The method doesn't guarantee to store all records, it just initiates
   * new send attempt. Synchronous. Can be interrupted from another thread.
   */
  void Flush() override;

  //! Dumps all buffer data and removes it from buffer.
  void Dump() override;

  DropCopyStrategyInstanceId RegisterStrategyInstance(const Strategy&) override;
  DropCopyStrategyInstanceId ContinueStrategyInstance(
      const Strategy&, const boost::posix_time::ptime&) override;
  DropCopyDataSourceInstanceId RegisterDataSourceInstance(
      const Strategy&,
      const boost::uuids::uuid& type,
      const boost::uuids::uuid& id) override;

  void CopySubmittedOrder(const OrderId&,
                          const boost::posix_time::ptime&,
                          const Security&,
                          const Lib::Currency&,
                          const TradingSystem&,
                          const OrderSide&,
                          const Qty&,
                          const boost::optional<Price>&,
                          const TimeInForce&) override;
  void CopySubmittedOrder(const OrderId&,
                          const boost::posix_time::ptime&,
                          const Position&,
                          const OrderSide&,
                          const Qty&,
                          const boost::optional<Price>&,
                          const TimeInForce&) override;
  void CopyOrderSubmitError(const boost::posix_time::ptime&,
                            const Security&,
                            const Lib::Currency&,
                            const TradingSystem&,
                            const OrderSide&,
                            const Qty&,
                            const boost::optional<Price>&,
                            const TimeInForce&,
                            const std::string& error) override;
  void CopyOrderSubmitError(const boost::posix_time::ptime&,
                            const Position&,
                            const OrderSide&,
                            const Qty&,
                            const boost::optional<Price>&,
                            const TimeInForce&,
                            const std::string& error) override;
  void CopyOrderStatus(const OrderId&,
                       const TradingSystem&,
                       const boost::posix_time::ptime&,
                       const OrderStatus&,
                       const Qty& remainingQty) override;
  void CopyOrder(const OrderId&,
                 const TradingSystem&,
                 const std::string& symbol,
                 const OrderStatus&,
                 const Qty& qty,
                 const Qty& remainingQty,
                 const boost::optional<Price>&,
                 const OrderSide&,
                 const TimeInForce&,
                 const boost::posix_time::ptime& openTime,
                 const boost::posix_time::ptime& updateTime) override;

  void CopyTrade(const boost::posix_time::ptime&,
                 const boost::optional<std::string>& tradingSystemTradeId,
                 const OrderId&,
                 const TradingSystem&,
                 const Price&,
                 const Qty&) override;

  void CopyOperationStart(const boost::uuids::uuid&,
                          const boost::posix_time::ptime&,
                          const Strategy&) override;
  void CopyOperationUpdate(const boost::uuids::uuid&,
                           const Pnl::Data&) override;
  void CopyOperationEnd(const boost::uuids::uuid&,
                        const boost::posix_time::ptime&,
                        std::unique_ptr<Pnl>&&) override;

  void CopyBook(const Security&, const PriceBook&) override;

  void CopyBar(const DropCopyDataSourceInstanceId&,
               size_t index,
               const boost::posix_time::ptime&,
               const Price& open,
               const Price& high,
               const Price& low,
               const Price& close) override;

  void CopyAbstractData(const DropCopyDataSourceInstanceId&,
                        size_t index,
                        const boost::posix_time::ptime&,
                        const Lib::Double& value) override;

  void CopyLevel1(const Security&,
                  const boost::posix_time::ptime&,
                  const Level1TickValue&) override;
  void CopyLevel1(const Security&,
                  const boost::posix_time::ptime&,
                  const Level1TickValue&,
                  const Level1TickValue&) override;
  void CopyLevel1(const Security&,
                  const boost::posix_time::ptime&,
                  const Level1TickValue&,
                  const Level1TickValue&,
                  const Level1TickValue&) override;
  void CopyLevel1(const Security&,
                  const boost::posix_time::ptime&,
                  const Level1TickValue&,
                  const Level1TickValue&,
                  const Level1TickValue&,
                  const Level1TickValue&) override;
  void CopyLevel1(const Security&,
                  const boost::posix_time::ptime&,
                  const std::vector<Level1TickValue>&) override;

  void CopyBalance(const TradingSystem&,
                   const std::string& symbol,
                   const Volume& available,
                   const Volume& locked) override;

 private:
  void SignalPriceUpdate(const Security&);

  const boost::posix_time::time_duration m_pollingInterval;
  boost::posix_time::ptime m_lastSignalTime;
};
}  // namespace FrontEnd
}  // namespace trdk
