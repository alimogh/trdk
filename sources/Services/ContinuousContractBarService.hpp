/**************************************************************************
 *   Created: 2016/09/12 01:05:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once
#include "Api.h"
#include "BarService.hpp"

namespace trdk {
namespace Services {

//! Bars collection service.
/** @sa
 * https://www.interactivebrokers.com/en/software/tws/usersguidebook/technicalanalytics/continuous.htm
  */
class TRDK_SERVICES_API ContinuousContractBarService : public BarService {
 public:
  typedef BarService Base;

 public:
  explicit ContinuousContractBarService(Context &context,
                                        const std::string &instanceName,
                                        const Lib::IniSectionRef &);
  virtual ~ContinuousContractBarService();

 public:
  virtual boost::posix_time::ptime GetLastDataTime() const override;

  virtual size_t GetSize() const override;
  virtual bool IsEmpty() const override;

  virtual const trdk::Security &GetSecurity() const override;

  virtual Bar GetBar(size_t index) const override;
  virtual Bar GetBarByReversedIndex(size_t index) const override;
  virtual Bar GetLastBar() const override;

  virtual void DropLastBarCopy(
      const trdk::DropCopyDataSourceInstanceId &) const override;
  virtual void DropUncompletedBarCopy(
      const trdk::DropCopyDataSourceInstanceId &) const override;

 public:
  virtual void OnSecurityStart(const trdk::Security &,
                               trdk::Security::Request &) override;

  virtual void OnSecurityContractSwitched(const boost::posix_time::ptime &,
                                          const trdk::Security &,
                                          trdk::Security::Request &,
                                          bool &isSwitched) override;

  virtual bool OnSecurityServiceEvent(const boost::posix_time::ptime &,
                                      const Security &,
                                      const Security::ServiceEvent &) override;

  virtual bool OnNewTrade(const trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::ScaledPrice &,
                          const trdk::Qty &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
