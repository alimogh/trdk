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
  explicit PositionController(trdk::Strategy &);
  virtual ~PositionController();

 public:
  trdk::Strategy &GetStrategy();
  const trdk::Strategy &GetStrategy() const;

 public:
  //! Handles trading signal event.
  /** @return Pointer of position object if new position is started or changed.
    *         nullptr - if no position was started or changed.
    */
  trdk::Position *OnSignal(trdk::Security &,
                           const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void OnPositionUpdate(trdk::Position &);
  void OnPostionsCloseRequest();
  void OnBrokerPositionUpdate(trdk::Security &,
                              bool isLong,
                              const trdk::Qty &,
                              const trdk::Volume &,
                              bool isInitial);

 public:
  virtual trdk::Position &OpenPosition(
      trdk::Security &, const trdk::Lib::TimeMeasurement::Milestones &);
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      bool isLong,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      bool isLong,
      const trdk::Qty &,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void ClosePosition(trdk::Position &, const trdk::CloseReason &);

 protected:
  boost::shared_ptr<Position> CreatePosition(
      bool isLong,
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);
  boost::shared_ptr<LongPosition> CreateLongPosition(
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);
  boost::shared_ptr<ShortPosition> CreateShortPosition(
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);

  template <typename PositionType>
  boost::shared_ptr<PositionType> CreatePositionObject(
      trdk::Security &security,
      const trdk::Qty &qty,
      const trdk::Price &price,
      const trdk::Lib::TimeMeasurement::Milestones &delayMeasurement) {
    const auto &result = boost::make_shared<PositionType>(
        GetStrategy(), GenerateNewOperationId(), 1, GetTradingSystem(security),
        security, security.GetSymbol().GetCurrency(), qty, price,
        delayMeasurement);
    return result;
  }
  virtual boost::shared_ptr<trdk::LongPosition> CreateLongPositionObject(
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual boost::shared_ptr<trdk::ShortPosition> CreateShortPositionObject(
      trdk::Security &,
      const trdk::Qty &,
      const trdk::Price &startPrice,
      const trdk::Lib::TimeMeasurement::Milestones &);

  void ContinuePosition(trdk::Position &);

  virtual std::unique_ptr<PositionReport> OpenReport() const;
  const PositionReport &GetReport() const;

 protected:
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy() const = 0;
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy() const = 0;
  virtual void SetupPosition(trdk::Position &) const = 0;
  virtual bool IsNewPositionIsLong() const = 0;
  virtual trdk::Qty GetNewPositionQty() const = 0;
  virtual bool IsPositionCorrect(const trdk::Position &) const = 0;

 private:
  boost::uuids::uuid GenerateNewOperationId() const;
  trdk::TradingSystem &GetTradingSystem(const trdk::Security &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}