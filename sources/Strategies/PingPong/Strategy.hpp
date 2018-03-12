/*******************************************************************************
 *   Created: 2018/01/31 18:43:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace PingPong {

class Strategy : public trdk::Strategy {
 public:
  struct Indicators {
    struct Ma {
      const char *name;
      std::unique_ptr<trdk::Lib::Accumulators::MovingAverage::Exponential> stat;
      size_t numberOfPeriods;

      explicit Ma(const char *name, size_t numberOfPeriods);
      void Reset(size_t numberOfPeriods);
    };
    struct Rsi {
      TradingLib::RelativeStrengthIndex stat;
      Lib::Double overboughtLevel;
      Lib::Double oversoldLevel;
    };

    Ma fastMa;
    Ma slowMa;

    Rsi rsi;

    explicit Indicators(size_t fastMaSize,
                        size_t slowMaSize,
                        size_t numberOfRsiPeriods,
                        const Lib::Double &overboughtLevel,
                        const Lib::Double &oversoldLevel);

    bool Update(const Price &);
  };

  struct IndicatorsToggles {
    struct Toggles {
      bool isOpeningSignalConfirmationEnabled;
      bool isClosingSignalConfirmationEnabled;
    };

    Toggles ma;
    Toggles rsi;
  };

  class Trend : public TradingLib::Trend {
   public:
    explicit Trend(const IndicatorsToggles::Toggles &);
    bool Update(
        const trdk::Lib::Accumulators::MovingAverage::Exponential &slowMa,
        const trdk::Lib::Accumulators::MovingAverage::Exponential &fastMa);
    bool Update(const Indicators::Rsi &);
    const IndicatorsToggles::Toggles &GetIndicatorsToggles() const;

   private:
    const IndicatorsToggles::Toggles &m_toggles;
  };

  struct Trends {
    Trend ma;
    Trend rsi;

   public:
    explicit Trends(const IndicatorsToggles &);
    bool Update(const Indicators &);
    bool IsRisingToOpen() const;
    bool HasCloseSignal(bool isLong) const;
  };

 public:
  typedef trdk::Strategy Base;

 public:
  explicit Strategy(Context &,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &);
  virtual ~Strategy() override;

 public:
  void Stop() noexcept;

  void EnableTrading(bool);
  bool IsTradingEnabled() const;

  void SetSourceTimeFrameSize(const boost::posix_time::time_duration &);

  void EnableActivePositionsControl(bool);
  bool IsActivePositionsControlEnabled() const;

  bool IsMaOpeningSignalConfirmationEnabled() const;
  void EnableMaOpeningSignalConfirmation(bool);
  bool IsMaClosingSignalConfirmationEnabled() const;
  void EnableMaClosingSignalConfirmation(bool);
  void SetNumberOfFastMaPeriods(size_t);
  size_t GetNumberOfFastMaPeriods() const;
  void SetNumberOfSlowMaPeriods(size_t);
  size_t GetNumberOfSlowMaPeriods() const;

  bool IsRsiOpeningSignalConfirmationEnabled() const;
  void EnableRsiOpeningSignalConfirmation(bool);
  bool IsRsiClosingSignalConfirmationEnabled() const;
  void EnableRsiClosingSignalConfirmation(bool);
  size_t GetNumberOfRsiPeriods() const;
  void SetNumberOfRsiPeriods(size_t);
  const Lib::Double &GetRsiOverboughtLevel() const;
  void SetRsiOverboughtLevel(const Lib::Double &);
  const Lib::Double &GetRsiOversoldLevel() const;
  void SetRsiOversoldLevel(const Lib::Double &);

  void SetPositionSize(const Qty &);
  const Qty &GetPositionSize() const;

  void SetStopLoss(const Lib::Double &);
  const Lib::Double &GetStopLoss() const;
  void SetTakeProfit(const Lib::Double &);
  void SetTakeProfitTrailing(const Lib::Double &);
  Lib::Double GetTakeProfit() const;
  Lib::Double GetTakeProfitTrailing() const;

  boost::signals2::scoped_connection SubscribeToEvents(
      const boost::function<void(const std::string &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

  const Trends &GetTrends(const Security &) const;

  void RaiseEvent(const std::string &);

  void EnableTradingSystem(size_t tradingSystemIndex, bool isEnabled);
  boost::tribool IsTradingSystemEnabled(size_t tradingSystemIndex) const;

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnLevel1Update(
      trdk::Security &, const Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  void OnPostionsCloseRequest() override;
  virtual bool OnBlocked(const std::string * = nullptr) noexcept override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace PingPong
}  // namespace Strategies
}  // namespace trdk