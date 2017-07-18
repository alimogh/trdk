/**************************************************************************
 *   Created: 2016/12/23 20:42:38
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
namespace Indicators {

class TRDK_SERVICES_API Rsi : public trdk::Service {
 public:
  //! General service error.
  class Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *) noexcept;
  };

  //! Throws when client code requests value which does not exist.
  class ValueDoesNotExistError : public Error {
   public:
    explicit ValueDoesNotExistError(const char *) noexcept;
  };

  //! Value data point.
  struct Point {
    boost::posix_time::ptime time;
    trdk::Lib::Double source;
    trdk::Lib::Double value;
  };

 public:
  explicit Rsi(Context &,
               const std::string &instanceName,
               const Lib::IniSectionRef &);
  virtual ~Rsi() noexcept;

 public:
  virtual const boost::posix_time::ptime &GetLastDataTime() const override;

  bool IsEmpty() const;

  //! Returns last value point.
  /** @throw ValueDoesNotExistError
    */
  const Point &GetLastPoint() const;

  //! Drops last value point copy.
  /** @throw RelativeStrengthIndexService
    */
  void DropLastPointCopy(const trdk::DropCopyDataSourceInstanceId &) const;

 protected:
  virtual bool OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}
