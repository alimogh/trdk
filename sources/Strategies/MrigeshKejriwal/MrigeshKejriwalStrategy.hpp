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

#include "Core/MarketDataSource.hpp"
#include "Core/Strategy.hpp"
#include "Services/MovingAverageService.hpp"
#include "Api.h"

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

////////////////////////////////////////////////////////////////////////////////

class TRDK_STRATEGY_MRIGESHKEJRIWAL_API Trend {
 public:
  Trend();
  virtual ~Trend() = default;

 public:
  virtual bool Update(const Price &lastPrice, double controlValue);

 public:
  //! Actual trend.
  /** @return True, if trend is "rising", false if trend is "falling", not
    *         true and not false if there is no trend at this moment.
    */
  virtual const boost::tribool &IsRising() const { return m_isRising; }

 private:
  boost::tribool m_isRising;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_STRATEGY_MRIGESHKEJRIWAL_API Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

 private:
  struct Settings {
    Qty qty;
    uint16_t numberOfHistoryHours;
    double costOfFunds;

    explicit Settings(const Lib::IniSectionRef &);

    void Validate() const;
    void Log(Module::Log &) const;
  };

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

  void OpenPosition(bool isLong,
                    const trdk::Lib::TimeMeasurement::Milestones &);

  template <typename PositionType>
  boost::shared_ptr<Position> CreatePosition(
      const trdk::ScaledPrice &price,
      const trdk::Qty &qty,
      const trdk::Lib::TimeMeasurement::Milestones &delayMeasurement) {
    return boost::make_shared<PositionType>(
        *this, m_generateUuid(), 1,
        GetTradingSystem(GetTradingSecurity().GetSource().GetIndex()),
        GetTradingSecurity(), GetTradingSecurity().GetSymbol().GetCurrency(),
        qty, price, delayMeasurement);
  }
  void ContinuePosition(trdk::Position &);
  void ClosePosition(trdk::Position &);

  void ReportOperation(const Position &);

  bool StartRollOver();
  void CancelRollOver();
  bool FinishRollOver();
  bool FinishRollOver(Position &oldPosition);

 private:
  const Settings m_settings;

  boost::uuids::random_generator m_generateUuid;

  trdk::Security *m_tradingSecurity;
  trdk::Security *m_underlyingSecurity;
  const trdk::Services::MovingAverageService *m_ma;

  boost::shared_ptr<Trend> m_trend;

  bool m_isRollover;

  std::ofstream m_strategyLog;
  bool m_isTradingSecurityActivationReported;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}