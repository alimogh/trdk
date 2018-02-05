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
namespace MarketMaker {

class TrendRepeatingStrategy : public trdk::Strategy {
 public:
  typedef Strategy Base;

 public:
  explicit TrendRepeatingStrategy(Context &,
                                  const std::string &instanceName,
                                  const Lib::IniSectionRef &);
  virtual ~TrendRepeatingStrategy() override;

 public:
  void EnableTrading(bool);
  bool IsTradingEnabled() const;

  void EnableActivePositionsControl(bool);
  bool IsActivePositionsControlEnabled() const;

  void SetNumberOfFastMaPeriods(size_t);
  size_t GetNumberOfFastMaPeriods() const;
  void SetNumberOfSlowMaPeriods(size_t);
  size_t GetNumberOfSlowMaPeriods() const;

  void SetPositionSize(const Qty &);
  Qty GetPositionSize() const;

  void SetStopLoss(const Lib::Double &);
  Lib::Double GetStopLoss() const;
  void SetTakeProfit(const Lib::Double &);
  Lib::Double GetTakeProfit() const;

  boost::signals2::scoped_connection SubscribeToEvents(
      const boost::function<void(const std::string &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

  const TradingLib::Trend &GetTrend() const;

  void ForEachSecurity(const boost::function<void(Security &)> &);

  void RaiseEvent(const std::string &);

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

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk