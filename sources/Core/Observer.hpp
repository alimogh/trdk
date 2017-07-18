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
  ~Observer();

 public:
  virtual void RaiseBrokerPositionUpdateEvent(trdk::Security &,
                                              const trdk::Qty &,
                                              bool isInitial);

  virtual void RaiseNewBarEvent(trdk::Security &, const trdk::Security::Bar &);
  virtual void RaiseSecurityServiceEvent(const boost::posix_time::ptime &,
                                         trdk::Security &,
                                         const trdk::Security::ServiceEvent &);

  void RaiseLevel1UpdateEvent(Security &);
  void RaiseLevel1TickEvent(trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &);
  void RaiseNewTradeEvent(trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::ScaledPrice &,
                          const trdk::Qty &);
  void RaiseServiceDataUpdateEvent(
      const trdk::Service &, const trdk::Lib::TimeMeasurement::Milestones &);

 protected:
  virtual void OnLevel1Update(trdk::Security &);
};
}
