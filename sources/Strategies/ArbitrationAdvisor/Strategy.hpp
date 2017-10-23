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

#include "Core/Strategy.hpp"
#include "Advice.hpp"
#include "Api.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class TRDK_STRATEGY_ARBITRATIONADVISOR_API Strategy : public trdk::Strategy {
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

  void ActivateAutoTrading(const Lib::Double &minPriceDifferenceRatio);
  const boost::optional<trdk::Lib::Double>
      &GetAutoTradingMinPriceDifferenceRatio() const;
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