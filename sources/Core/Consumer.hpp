/**************************************************************************
 *   Created: 2013/05/14 07:19:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Module.hpp"

namespace trdk {

class TRDK_CORE_API Consumer : public trdk::Module {
 public:
  explicit Consumer(trdk::Context &,
                    const std::string &typeName,
                    const std::string &name,
                    const std::string &instanceName,
                    const trdk::Lib::IniSectionRef &);
  virtual ~Consumer();

 public:
  void RegisterSource(trdk::Security &);

  SecurityList &GetSecurities();
  const SecurityList &GetSecurities() const;

 public:
  virtual void RaiseSecurityContractSwitchedEvent(
      const boost::posix_time::ptime &,
      trdk::Security &,
      trdk::Security::Request &) = 0;

  virtual void RaiseBrokerPositionUpdateEvent(trdk::Security &,
                                              const trdk::Qty &,
                                              bool isInitial) = 0;

  virtual void RaiseSecurityServiceEvent(
      const boost::posix_time::ptime &,
      trdk::Security &,
      const trdk::Security::ServiceEvent &) = 0;

 protected:
  //! Notifies about new security start.
  virtual void OnSecurityStart(trdk::Security &, trdk::Security::Request &);

  //! Notifies when security switched to another contract.
  /** All marked data for security will be reset (so if security just
    * started).
    */
  virtual void OnSecurityContractSwitched(const boost::posix_time::ptime &,
                                          trdk::Security &,
                                          trdk::Security::Request &);

  virtual void OnLevel1Tick(trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &);

  virtual void OnNewTrade(trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::ScaledPrice &,
                          const trdk::Qty &);

  virtual void OnServiceDataUpdate(
      const trdk::Service &, const trdk::Lib::TimeMeasurement::Milestones &);

  //! Notifies about broker position update.
  /** @sa trdk::Security::GetBrokerPosition
    * @param security Security.
    * @param qty        Position size. Negative value means "short position",
    *                   positive - "long position". May differ from current
    *                   trdk::Security::GetBrokerPosition.
    * @param isInitial  true if it initial data at start.
    */
  virtual void OnBrokerPositionUpdate(trdk::Security &,
                                      const trdk::Qty &,
                                      bool isInitial);

  virtual void OnNewBar(trdk::Security &, const trdk::Security::Bar &);

  virtual void OnSecurityServiceEvent(const boost::posix_time::ptime &,
                                      trdk::Security &,
                                      const trdk::Security::ServiceEvent &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
