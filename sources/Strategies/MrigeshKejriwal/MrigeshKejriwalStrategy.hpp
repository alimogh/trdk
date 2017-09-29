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
  class OrderPolicyFactory : private boost::noncopyable {
   public:
    virtual ~OrderPolicyFactory() = default;
    virtual std::unique_ptr<TradingLib::OrderPolicy> CreateOrderPolicy()
        const = 0;
  };

  Qty qty;
  Qty minQty;
  uint16_t numberOfHistoryHours;
  Lib::Double costOfFunds;
  Lib::Double maxLossShare;
  Price signalPriceCorrection;
  std::unique_ptr<OrderPolicyFactory> orderPolicyFactory;

  explicit Settings(const Lib::IniSectionRef &);

  void Validate() const;
  void Log(Module::Log &) const;

 private:
  class LimitGtcOrderPolicyFactory;
  class LimitIocOrderPolicyFactory;
  class MarketGtcOrderPolicyFactory;
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

 protected:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override;
  virtual void SetupPosition(trdk::Position &) const override;
  virtual bool IsNewPositionIsLong() const override;
  virtual trdk::Qty GetNewPositionQty() const override;
  virtual bool IsPositionCorrect(const Position &position) const override;

  virtual std::unique_ptr<TradingLib::PositionReport> OpenReport()
      const override;

 private:
  const Settings &m_settings;
  const boost::shared_ptr<const TradingLib::OrderPolicy> m_orderPolicy;
  const Trend &m_trend;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_STRATEGY_MRIGESHKEJRIWAL_API Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

 private:
  struct Rollover {
    bool isStarted;
    bool isLong;
    trdk::Qty qty;
    bool isCompleted;
  };

  struct PriceSignal {
    bool isLong;
    Price price;
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
                                          trdk::Security::Request &,
                                          bool &isSwitched) override;
  virtual void OnBrokerPositionUpdate(trdk::Security &,
                                      bool isLong,
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
  bool ContinueRollOver();
  bool FinishRollOver();

 private:
  const Settings m_settings;
  bool m_skipNextSignal;

  trdk::Security *m_tradingSecurity;
  trdk::Security *m_underlyingSecurity;
  const trdk::Services::MovingAverageService *m_ma;

  boost::shared_ptr<Trend> m_trend;
  PositionController m_positionController;

  boost::optional<Rollover> m_rollover;

  boost::optional<PriceSignal> m_priceSignal;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}