/*******************************************************************************
 *   Created: 2018/02/20 23:57:04
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

class TakerStrategy : public Strategy {
 public:
  typedef Strategy Base;

 public:
  explicit TakerStrategy(Context &,
                         const std::string &instanceName,
                         const Lib::IniSectionRef &);
  virtual ~TakerStrategy() override;

 public:
  void Stop() noexcept;

  void EnableTrading(bool);
  bool IsTradingEnabled() const;

  void SetNumerOfPeriods(size_t);
  size_t GetNumerOfPeriods() const;

  void SetGoalVolume(const Volume &);
  Volume GetGoalVolume() const;

  void SetMaxLoss(const Volume &);
  Volume GetMaxLoss() const;

  void SetPeriodSize(size_t numberOfMinutes);
  size_t GetPeriodSize() const;

  void SetMinPrice(const Price &);
  Price GetMinPrice() const;
  void SetMaxPrice(const Price &);
  Price GetMaxPrice() const;

  void SetTradeMinVolume(const Volume &);
  Volume GetTradeMinVolume() const;
  void SetTradeMaxVolume(const Volume &);
  Volume GetTradeMaxVolume() const;

  boost::signals2::scoped_connection SubscribeToCompleted(
      const boost::function<void()> &);
  boost::signals2::scoped_connection SubscribeToPnl(
      const boost::function<void(const Volume &, const Volume &)> &);
  boost::signals2::scoped_connection SubscribeToVolume(
      const boost::function<void(const Volume &, const Volume &)> &);
  boost::signals2::scoped_connection SubscribeToEvents(
      const boost::function<void(const std::string &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

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

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk