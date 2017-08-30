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
  void OnPositionUpdate(trdk::Position &);

 public:
  virtual trdk::Position &OpenPosition(
      trdk::Security &, const trdk::Lib::TimeMeasurement::Milestones &) = 0;
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      bool isLong,
      const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void ContinuePosition(trdk::Position &) = 0;
  virtual void ClosePosition(trdk::Position &, const trdk::CloseReason &) = 0;

 protected:
  template <typename PositionType>
  boost::shared_ptr<Position> CreatePosition(
      trdk::Security &security,
      const trdk::ScaledPrice &price,
      const trdk::Lib::TimeMeasurement::Milestones &delayMeasurement) {
    return boost::make_shared<PositionType>(
        GetStrategy(), GenerateNewOperationId(), 1, GetTradingSystem(security),
        security, security.GetSymbol().GetCurrency(), 1, price,
        delayMeasurement);
  }

  virtual std::unique_ptr<PositionReport> OpenReport() const;

 protected:
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