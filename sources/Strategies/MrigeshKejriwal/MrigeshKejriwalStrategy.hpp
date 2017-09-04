/*******************************************************************************
 *   Created: 2017/07/22 21:47:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "TradingLib/PositionController.hpp"
#include "TradingLib/Trend.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Strategy.hpp"
#include "Services/MovingAverageService.hpp"
#include "Api.h"

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

////////////////////////////////////////////////////////////////////////////////

struct Settings {
  Qty qty;
  Qty minQty;
  uint16_t numberOfHistoryHours;
  double costOfFunds;
  double maxLossShare;

  explicit Settings(const Lib::IniSectionRef &);

  void Validate() const;
  void Log(Module::Log &) const;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_STRATEGY_MRIGESHKEJRIWAL_API Trend : public TradingLib::Trend {
 public:
  virtual ~Trend() = default;

 public:
  virtual bool Update(const Price &lastPrice, double controlValue);
};

////////////////////////////////////////////////////////////////////////////////

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(Strategy &, const Trend &, const Settings &);
  virtual ~PositionController() override = default;

 public:
  virtual Position &OpenPosition(
      Security &, const Lib::TimeMeasurement::Milestones &) override;
  virtual Position &OpenPosition(
      Security &,
      bool isLong,
      const Lib::TimeMeasurement::Milestones &) override;
  virtual void ContinuePosition(Position &) override;
  virtual void ClosePosition(Position &, const CloseReason &) override;

 protected:
  virtual trdk::Qty GetNewPositionQty() const override;
  virtual bool IsPositionCorrect(const Position &position) const override;

 private:
  const Trend &m_trend;
  const Settings &m_settings;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_STRATEGY_MRIGESHKEJRIWAL_API Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

 public:
  explicit Strategy(
      trdk::Context &,
      const std::string &instanceName,
      const trdk::Lib::IniSectionRef &,
      const boost::shared_ptr<Trend> &trend = boost::make_shared<Trend>());
  virtual ~Strategy() override = default;

 public:
  virtual void OnServiceStart(const trdk::Service &) override;

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnSecurityContractSwitched(const boost::posix_time::ptime &,
                                          trdk::Security &,
                                          trdk::Security::Request &) override;
  virtual void OnBrokerPositionUpdate(trdk::Security &,
                                      const trdk::Qty &,
                                      const trdk::Volume &,
                                      bool isInitial) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  virtual void OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPostionsCloseRequest() override;

  virtual void OnLevel1Tick(
      trdk::Security &,
      const boost::posix_time::ptime &,
      const trdk::Level1TickValue &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;

 private:
  trdk::Security &GetTradingSecurity() {
    TrdkAssert(m_tradingSecurity);
    return *m_tradingSecurity;
  }
  const trdk::Security &GetUnderlyingSecurity() const {
    TrdkAssert(m_underlyingSecurity);
    return *m_underlyingSecurity;
  }
  const trdk::Services::MovingAverageService &GetMa() const {
    TrdkAssert(m_ma);
    return *m_ma;
  }

  void OnMaServiceStart(const trdk::Services::MovingAverageService &);

  void CheckSignal(const trdk::Price &tradePrice,
                   const trdk::Lib::TimeMeasurement::Milestones &);

  bool StartRollOver();
  void CancelRollOver();
  bool FinishRollOver();
  bool FinishRollOver(Position &oldPosition);

 private:
  const Settings m_settings;

  trdk::Security *m_tradingSecurity;
  trdk::Security *m_underlyingSecurity;
  const trdk::Services::MovingAverageService *m_ma;

  boost::shared_ptr<Trend> m_trend;
  PositionController m_positionController;

  bool m_isRollover;

  bool m_isTradingSecurityActivationReported;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}