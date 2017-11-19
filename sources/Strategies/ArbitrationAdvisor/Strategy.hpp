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
    trdk::Lib::Double minPriceDifferenceRatio;
    trdk::Qty maxQty;
  };

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

 public:
  void SetupAdvising(const Lib::Double &minPriceDifferenceRatio) const;

  void ActivateAutoTrading(TradingSettings &&);
  const boost::optional<TradingSettings> &GetAutoTradingSettings() const;
  void DeactivateAutoTrading();

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnLevel1Update(
      trdk::Security &, const Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  void OnPostionsCloseRequest() override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}