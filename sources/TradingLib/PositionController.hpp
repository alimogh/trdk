/*******************************************************************************
 *   Created: 2017/08/26 19:03:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk {
namespace TradingLib {

class PositionController : private boost::noncopyable {
 public:
  virtual ~PositionController() = default;

  //! Handles trading signal event.
  /**
   * Works only with one active position per strategy instance.
   * @param[in] newOperationContext  New position operation context. Will be
   *                                 used only if new position instance will be
   *                                 created.
   * @param[in] subOperationId       New position operation sub-ID. Will be
   *                                 used only if new position instance will be
   *                                 created.
   * @param[in] security             New position security. Will be used only if
   *                                 new position instance will be created.
   * @param[int] delayMeasurement    Delay measurement.
   * @return A pointer to position object if a new position is started or if an
   *         existing position is changed. nullptr - if no position was
   *         started or changed.
   */
  Position* OnSignal(const boost::shared_ptr<Operation>& newOperationContext,
                     int64_t subOperationId,
                     Security& security,
                     const Lib::TimeMeasurement::Milestones& delayMeasurement);
  virtual void OnUpdate(Position&);
  virtual void OnCloseAllRequest(Strategy&);
  void OnBrokerUpdate(const boost::shared_ptr<Operation>&,
                      int64_t subOperationId,
                      Security&,
                      bool isLong,
                      const Qty&,
                      const Volume&,
                      bool isInitial);

  virtual Position* Open(const boost::shared_ptr<Operation>&,
                         int64_t subOperationId,
                         Security&,
                         const Lib::TimeMeasurement::Milestones&);
  virtual Position* Open(const boost::shared_ptr<Operation>&,
                         int64_t subOperationId,
                         Security&,
                         bool isLong,
                         const Lib::TimeMeasurement::Milestones&);
  virtual Position* Open(const boost::shared_ptr<Operation>&,
                         int64_t subOperationId,
                         Security&,
                         bool isLong,
                         const Qty&,
                         const Lib::TimeMeasurement::Milestones&);
  virtual Position& Restore(
      const boost::shared_ptr<Operation>&,
      int64_t subOperationId,
      Security&,
      bool isLong,
      const boost::posix_time::ptime& openStartTime,
      const boost::posix_time::ptime& openTime,
      const Qty&,
      const Price& startPrice,
      const Price& openPrice,
      const boost::shared_ptr<const OrderTransactionContext>& openingContext,
      const Lib::TimeMeasurement::Milestones&);

  //! Cancels active open-order, if exists, and closes position.
  /**
   * @return true, if did some action to close position, if not (for ex.: there
   *         is no opened position) - false.
   */
  virtual bool Close(Position&, const CloseReason&);
  //! Cancels all active open-orders, if exists, and closes all positions.
  void CloseAll(Strategy&, const CloseReason&);

 protected:
  boost::shared_ptr<Position> Create(const boost::shared_ptr<Operation>&,
                                     int64_t subOperationId,
                                     bool isLong,
                                     Security&,
                                     const Qty&,
                                     const Price& startPrice,
                                     const Lib::TimeMeasurement::Milestones&);

  static void Continue(Position&);

  //! Will be called one time when the position is fully opened.
  /** The default implementation does nothing.
   */
  virtual void Hold(Position&);
  //! Will be called one time when the position opened not fully and the trading
  //! system doesn't allow to send last order to open position fully.
  /**
   * The default implementation calls Continue even if the order will be
   * rejected by the trading system as the order will have wrong parameters
   *
   * @sa Continue
   */
  virtual void Hold(Position&, const OrderCheckError&);

  virtual void Close(Position&);

  //! Will be called one time when the position is fully closed.
  /** The default implementation does nothing.
   */
  virtual void Complete(Position&);
  //! Will be called one time when the position closed not fully and the trading
  //! system doesn't allow to send last order to close position fully.
  /**
   * The default implementation calls Close, even if it will be rejected by the
   * trading system as the order will have wrong parameters.
   *
   * @sa Close
   */
  virtual void Complete(Position&, const OrderCheckError&);

  virtual Position* GetExisting(Strategy&, Security&);
};
}  // namespace TradingLib
}  // namespace trdk
