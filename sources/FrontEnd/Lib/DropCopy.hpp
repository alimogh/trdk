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

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API DropCopy : public QObject, public trdk::DropCopy {
  Q_OBJECT

 public:
  explicit DropCopy(QObject *parent);
  DropCopy(DropCopy &&) = delete;
  DropCopy(const DropCopy &) = delete;
  DropCopy &operator=(DropCopy &&) = delete;
  DropCopy &operator=(const DropCopy &) = delete;
  ~DropCopy() override = default;

 signals:
  void PriceUpdated(const Security *);

  void FreeOrderSubmited(const QString &remoteId,
                         const QDateTime &,
                         const Security *,
                         const boost::shared_ptr<const Lib::Currency> &,
                         const TradingSystem *,
                         const boost::shared_ptr<const OrderSide> &,
                         const Qty &,
                         const boost::optional<Price> &,
                         const TimeInForce &);
  void OperationOrderSubmited(const QUuid &operationId,
                              int64_t subOperationId,
                              const QString &orderRemoteId,
                              const QDateTime &,
                              const Security *,
                              const boost::shared_ptr<const Lib::Currency> &,
                              const TradingSystem *,
                              const boost::shared_ptr<const OrderSide> &,
                              const Qty &,
                              const boost::optional<Price> &,
                              const TimeInForce &);
  void FreeOrderSubmitError(const QDateTime &,
                            const Security *,
                            const boost::shared_ptr<const Lib::Currency> &,
                            const TradingSystem *,
                            const boost::shared_ptr<const OrderSide> &,
                            const Qty &,
                            const boost::optional<Price> &,
                            const TimeInForce &,
                            const QString &error);
  void OperationOrderSubmitError(const QUuid &operationId,
                                 int64_t subOperationId,
                                 const QDateTime &,
                                 const Security *,
                                 const boost::shared_ptr<const Lib::Currency> &,
                                 const TradingSystem *,
                                 const boost::shared_ptr<const OrderSide> &,
                                 const Qty &,
                                 const boost::optional<Price> &,
                                 const TimeInForce &,
                                 const QString &error);
  void OrderUpdated(const QString &remoteId,
                    const TradingSystem *,
                    const QDateTime &,
                    const boost::shared_ptr<const OrderStatus> &,
                    const Qty &remainingQty);

  void BalanceUpdated(const TradingSystem *,
                      const QString &symbol,
                      const Volume &available,
                      const Volume &locked);

  void OperationStarted(const QUuid &, const QDateTime &, const Strategy *);
  void OperationUpdated(const QUuid &, const Pnl::Data &);
  void OperationCompleted(const QUuid &,
                          const QDateTime &,
                          const boost::shared_ptr<const Pnl> &);

  void BarUpdated(const Security *, const Bar &);

 public:
  void Flush() override;

  void Dump() override;

  void CopySubmittedOrder(const OrderId &,
                          const boost::posix_time::ptime &,
                          const Security &,
                          const Lib::Currency &,
                          const TradingSystem &,
                          const trdk::OrderSide &,
                          const Qty &,
                          const boost::optional<Price> &,
                          const TimeInForce &) override;
  void CopySubmittedOrder(const OrderId &,
                          const boost::posix_time::ptime &,
                          const Position &,
                          const trdk::OrderSide &,
                          const Qty &,
                          const boost::optional<Price> &,
                          const TimeInForce &) override;
  void CopyOrderSubmitError(const boost::posix_time::ptime &,
                            const Security &,
                            const Lib::Currency &,
                            const TradingSystem &,
                            const trdk::OrderSide &,
                            const Qty &,
                            const boost::optional<Price> &,
                            const TimeInForce &,
                            const std::string &error) override;
  void CopyOrderSubmitError(const boost::posix_time::ptime &,
                            const Position &,
                            const trdk::OrderSide &,
                            const Qty &,
                            const boost::optional<Price> &,
                            const TimeInForce &,
                            const std::string &error) override;
  void CopyOrderStatus(const OrderId &,
                       const TradingSystem &,
                       const boost::posix_time::ptime &,
                       const trdk::OrderStatus &,
                       const Qty &remainingQty) override;

  void CopyTrade(const boost::posix_time::ptime &,
                 const boost::optional<std::string> &tradingSystemTradeId,
                 const OrderId &,
                 const TradingSystem &,
                 const Price &,
                 const Qty &) override;

  void CopyOperationStart(const boost::uuids::uuid &,
                          const boost::posix_time::ptime &,
                          const Strategy &) override;
  void CopyOperationUpdate(const boost::uuids::uuid &,
                           const Pnl::Data &) override;
  void CopyOperationEnd(const boost::uuids::uuid &,
                        const boost::posix_time::ptime &,
                        std::unique_ptr<Pnl> &&) override;

  void CopyBook(const Security &, const PriceBook &) override;

  void CopyBar(const Security &, const Bar &) override;

  void CopyLevel1(const Security &,
                  const boost::posix_time::ptime &,
                  const Level1TickValue &) override;
  void CopyLevel1(const Security &,
                  const boost::posix_time::ptime &,
                  const Level1TickValue &,
                  const Level1TickValue &) override;
  void CopyLevel1(const Security &,
                  const boost::posix_time::ptime &,
                  const Level1TickValue &,
                  const Level1TickValue &,
                  const Level1TickValue &) override;
  void CopyLevel1(const Security &,
                  const boost::posix_time::ptime &,
                  const Level1TickValue &,
                  const Level1TickValue &,
                  const Level1TickValue &,
                  const Level1TickValue &) override;
  void CopyLevel1(const Security &,
                  const boost::posix_time::ptime &,
                  const std::vector<Level1TickValue> &) override;

  void CopyBalance(const TradingSystem &,
                   const std::string &symbol,
                   const Volume &available,
                   const Volume &locked) override;

 private:
  void SignalPriceUpdate(const Security &);

  const boost::posix_time::time_duration m_pollingInterval;
  boost::posix_time::ptime m_lastSignalTime;
};

}  // namespace FrontEnd
}  // namespace trdk
