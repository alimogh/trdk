/**************************************************************************
 *   Created: 2012/09/19 23:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Observer.hpp"
#include "Service.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

Observer::Observer(Context &context,
                   const std::string &implementationName,
                   const std::string &instanceName,
                   const IniSectionRef &conf)
    : Consumer(context, "Observer", implementationName, instanceName, conf) {}

Observer::~Observer() {}

void Observer::OnLevel1Update(Security &security) {
  GetLog().Error(
      "Subscribed to %1% level 1 updates, but can't work with it"
      " (doesn't have OnLevel1Update method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Module subscribed to level 1 updates, but can't work with it");
}

void Observer::RaiseBrokerPositionUpdateEvent(Security &security,
                                              const Qty &qty,
                                              const Volume &volume,
                                              bool isInitial) {
  const auto lock = LockForOtherThreads();
  OnBrokerPositionUpdate(security, qty, volume, isInitial);
}

void Observer::RaiseNewBarEvent(Security &security, const Security::Bar &bar) {
  const auto lock = LockForOtherThreads();
  return OnNewBar(security, bar);
}

void Observer::RaiseLevel1UpdateEvent(Security &security) {
  const auto lock = LockForOtherThreads();
  OnLevel1Update(security);
}

void Observer::RaiseLevel1TickEvent(
    Security &security,
    const boost::posix_time::ptime &time,
    const Level1TickValue &value,
    const TimeMeasurement::Milestones &delayMeasurement) {
  const auto lock = LockForOtherThreads();
  OnLevel1Tick(security, time, value, delayMeasurement);
}

void Observer::RaiseNewTradeEvent(Security &security,
                                  const boost::posix_time::ptime &time,
                                  const trdk::Price &price,
                                  const trdk::Qty &qty) {
  const auto lock = LockForOtherThreads();
  OnNewTrade(security, time, price, qty);
}

void Observer::RaiseServiceDataUpdateEvent(
    const Service &service,
    const TimeMeasurement::Milestones &delayMeasurement) {
  const auto lock = LockForOtherThreads();
  OnServiceDataUpdate(service, delayMeasurement);
}

void Observer::RaiseSecurityServiceEvent(
    const pt::ptime &time,
    Security &security,
    const Security::ServiceEvent &securityEvent) {
  const auto lock = LockForOtherThreads();
  OnSecurityServiceEvent(time, security, securityEvent);
}
