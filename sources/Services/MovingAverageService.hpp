/**************************************************************************
 *   Created: 2013/10/23 19:44:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"
#include "BarService.hpp"

namespace trdk {
namespace Services {

class TRDK_SERVICES_API MovingAverageService : public trdk::Service {
 public:
  //! General service error.
  class Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *) throw();
  };

  //! Throws when client code requests value which does not exist.
  class ValueDoesNotExistError : public Error {
   public:
    explicit ValueDoesNotExistError(const char *) throw();
  };

  //! Service has not points history.
  class HasNotHistory : public Error {
   public:
    explicit HasNotHistory(const char *) throw();
  };

  //! Value data point.
  struct Point {
    boost::posix_time::ptime time;
    double source;
    double value;
  };

 public:
  explicit MovingAverageService(Context &,
                                const std::string &instanceName,
                                const Lib::IniSectionRef &);
  virtual ~MovingAverageService();

 public:
  virtual const boost::posix_time::ptime &GetLastDataTime() const override;

  bool IsEmpty() const;

  //! Returns last value point.
  /** @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
    */
  const Point &GetLastPoint() const;

  //! Drops last value point copy.
  /** @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
    */
  void DropLastPointCopy(const trdk::DropCopyDataSourceInstanceId &) const;

 public:
  //! Number of points from history.
  size_t GetHistorySize() const;

  //! Returns value point from history by index.
  /** First value has index "zero".
    * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
    * @throw trdk::Services::MovingAverageService::HasNotHistory
    * @sa trdk::Services::MovingAverageService::GetValueByReversedIndex
    */
  const Point &GetHistoryPoint(size_t index) const;

  //! Returns value point from history by reversed index.
  /** Last value has index "zero".
    * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
    * @throw trdk::Services::MovingAverageService::HasNotHistory
    * @sa trdk::Services::MovingAverageService::GetLastPoint
    */
  const Point &GetHistoryPointByReversedIndex(size_t index) const;

 protected:
  virtual void OnSecurityContractSwitched(const boost::posix_time::ptime &,
                                          const trdk::Security &,
                                          trdk::Security::Request &) override;

  virtual bool OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;

 public:
  virtual bool OnLevel1Tick(const trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &) override;

  virtual bool OnNewBar(const trdk::Security &,
                        const trdk::Services::BarService::Bar &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

namespace Indicators {
//! Moving Average.
typedef trdk::Services::MovingAverageService Ma;
}
}
}
