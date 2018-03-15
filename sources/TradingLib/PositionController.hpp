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

 public:
  //! Handles trading signal event.
  /** Works only with one active position per strategy instance.
   * @param newOperationContext  New position operation context. Will be used
   *                             only if new position instance will be created.
   * @param security             New position security. Will be used only if
   *                             new position instance will be created.
   * @return A pointer to position object if a new position is started or if an
   *         existing position is changed. nullptr - if no position was
   *         started or changed.
   */
  trdk::Position *OnSignal(
      const boost::shared_ptr<trdk::Operation> &newOperationContext,
      int64_t subOperationId,
      trdk::Security &security,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void OnPositionUpdate(trdk::Position &);
  virtual void OnPostionsCloseRequest(trdk::Strategy &);
  void OnBrokerPositionUpdate(const boost::shared_ptr<trdk::Operation> &,
                              int64_t subOperationId,
                              trdk::Security &,
                              bool isLong,
                              const trdk::Qty &,
                              const trdk::Volume &,
                              bool isInitial);

 public:
  virtual trdk::Position *OpenPosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual trdk::Position *OpenPosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      bool isLong,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual trdk::Position *OpenPosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      bool isLong,
      const trdk::Qty &,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual trdk::Position &RestorePosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      bool isLong,
      const boost::posix_time::ptime &openStartTime,
      const boost::posix_time::ptime &openTime,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Price &openPrice,
      const boost::shared_ptr<const trdk::OrderTransactionContext>
          &openingContext,
      const trdk::Lib::TimeMeasurement::Milestones &);

  //! Cancels active open-order, if exists, and closes position.
  /** @return true, if did some action to close position, if not (for ex.: there
   *         is no opened position) - false.
   */
  virtual bool ClosePosition(trdk::Position &, const trdk::CloseReason &);
  //! Cancels all active open-orders, if exists, and closes all positions.
  void CloseAllPositions(trdk::Strategy &, const trdk::CloseReason &);

 protected:
  boost::shared_ptr<Position> CreatePosition(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      bool isLong,
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);

  void ContinuePosition(trdk::Position &);
  virtual void ClosePosition(trdk::Position &);

  virtual void HoldPosition(trdk::Position &);

  virtual trdk::Position *GetExistingPosition(trdk::Strategy &,
                                              trdk::Security &);

 private:
  template <typename PositionType>
  boost::shared_ptr<trdk::Position> CreatePositionObject(
      const boost::shared_ptr<trdk::Operation> &,
      int64_t subOperationId,
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::Lib::TimeMeasurement::Milestones &);
};
}  // namespace TradingLib
}  // namespace trdk