/*******************************************************************************
 *   Created: 2017/10/12 11:27:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Strategy.hpp"
#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace Strategies {
namespace WilliamCarry {

class TRDK_STRATEGY_WILLIAMCARRY_API Multibroker : public Strategy {
 public:
  typedef Strategy Base;

 public:
  explicit Multibroker(Context &,
                       const std::string &instanceName,
                       const Lib::IniSectionRef &);
  virtual ~Multibroker() override;

 public:
  void OpenPosition(OperationContext &&,
                    Security &,
                    const Lib::TimeMeasurement::Milestones &);

 protected:
  virtual void OnLevel1Tick(
      trdk::Security &,
      const boost::posix_time::ptime &,
      const trdk::Level1TickValue &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  void OnPostionsCloseRequest() override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}
