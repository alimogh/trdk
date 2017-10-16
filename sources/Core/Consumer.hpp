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
  virtual ~Consumer() override;

 public:
  void RegisterSource(trdk::Security &);

  SecurityList &GetSecurities();
  const SecurityList &GetSecurities() const;

 public:
  virtual void RaiseSecurityContractSwitchedEvent(
      const boost::posix_time::ptime &,
      trdk::Security &,
      trdk::Security::Request &,
      bool &isSwitched) = 0;

  virtual void RaiseBrokerPositionUpdateEvent(trdk::Security &,
                                              bool isLong,
                                              const trdk::Qty &,
                                              const trdk::Volume &,
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
                                          trdk::Security::Request &,
                                          bool &isSwitched);

  virtual void OnLevel1Tick(trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &,
                            const trdk::Lib::TimeMeasurement::Milestones &);

  virtual void OnNewTrade(trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Price &,
                          const trdk::Qty &);

  virtual void OnServiceDataUpdate(
      const trdk::Service &, const trdk::Lib::TimeMeasurement::Milestones &);

  //! Notifies about broker position update.
  /** @param[in, out] security  Security.
    *  @param[in]     isLong    If true - position has type "long", "short"
    *                           otherwise.
    * @param[in]      qty       Position size.
    * @param[in]      volume    Position volume.
    * @param[in]      isInitial true if it initial data at start.
    */
  virtual void OnBrokerPositionUpdate(trdk::Security &security,
                                      bool isLong,
                                      const trdk::Qty &qty,
                                      const trdk::Volume &volume,
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
