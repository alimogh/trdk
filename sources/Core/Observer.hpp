/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Consumer.hpp"

namespace trdk {

class TRDK_CORE_API Observer : public trdk::Consumer {
 public:
  Observer(trdk::Context &,
           const std::string &implementationName,
           const std::string &instanceName,
           const trdk::Lib::IniSectionRef &);
  virtual ~Observer() override;

 public:
  virtual void RaiseBrokerPositionUpdateEvent(trdk::Security &,
                                              bool isLong,
                                              const trdk::Qty &,
                                              const trdk::Volume &,
                                              bool isInitial) override;

  virtual void RaiseNewBarEvent(trdk::Security &, const trdk::Security::Bar &);
  virtual void RaiseSecurityServiceEvent(
      const boost::posix_time::ptime &,
      trdk::Security &,
      const trdk::Security::ServiceEvent &) override;

  void RaiseLevel1UpdateEvent(Security &);
  void RaiseLevel1TickEvent(trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &,
                            const trdk::Lib::TimeMeasurement::Milestones &);
  void RaiseNewTradeEvent(trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Price &,
                          const trdk::Qty &);
  void RaiseServiceDataUpdateEvent(
      const trdk::Service &, const trdk::Lib::TimeMeasurement::Milestones &);

 protected:
  virtual void OnLevel1Update(trdk::Security &);
};
}
