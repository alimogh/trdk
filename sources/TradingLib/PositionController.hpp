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
  void OnSignal(trdk::Security &,
                const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void OnPositionUpdate(trdk::Position &);
  void OnPostionsCloseRequest();
  void OnBrokerPositionUpdate(trdk::Security &,
                              const trdk::Qty &,
                              const trdk::Volume &,
                              bool isInitial);

 public:
  virtual trdk::Position &OpenPosition(
      trdk::Security &, const trdk::Lib::TimeMeasurement::Milestones &) = 0;
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
  void ContinuePosition(trdk::Position &);

  template <typename PositionType>
  boost::shared_ptr<Position> CreatePosition(
      trdk::Security &security,
      const trdk::Qty &qty,
      const trdk::ScaledPrice &price,
      const trdk::Lib::TimeMeasurement::Milestones &delayMeasurement) {
    return boost::make_shared<PositionType>(
        GetStrategy(), GenerateNewOperationId(), 1, GetTradingSystem(security),
        security, security.GetSymbol().GetCurrency(), qty, price,
        delayMeasurement);
  }

  virtual std::unique_ptr<PositionReport> OpenReport() const;
  const PositionReport &GetReport() const;

 protected:
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy() const = 0;
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy() const = 0;
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