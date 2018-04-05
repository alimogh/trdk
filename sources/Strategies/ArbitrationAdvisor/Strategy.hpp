/*******************************************************************************
 *   Created: 2017/10/22 17:53:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Advice.hpp"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class Strategy : public trdk::Strategy {
 public:
  struct TradingSettings {
    bool isEnabled;
    trdk::Lib::Double minPriceDifferenceRatio;
    trdk::Qty maxQty;
  };

 public:
  static const boost::uuids::uuid typeId;

 public:
  typedef trdk::Strategy Base;

 public:
  explicit Strategy(Context &,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &);
  virtual ~Strategy() override;

 public:
  boost::signals2::scoped_connection SubscribeToAdvice(
      const boost::function<void(const Advice &)> &);
  boost::signals2::scoped_connection SubscribeToTradingSignalCheckErrors(
      const boost::function<void(const std::vector<std::string> &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

 public:
  void SetMinPriceDifferenceRatioToAdvice(const Lib::Double &);
  const Lib::Double &GetMinPriceDifferenceRatioToAdvice() const;

  void SetTradingSettings(TradingSettings &&);
  const TradingSettings &GetTradingSettings() const;

  bool IsLowestSpreadEnabled() const;
  const Lib::Double &GetLowestSpreadRatio() const;

  bool IsStopLossEnabled() const;
  const boost::posix_time::time_duration &GetStopLossDelay() const;

  void ForEachSecurity(const Lib::Symbol &,
                       const boost::function<void(Security &)> &);

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
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk