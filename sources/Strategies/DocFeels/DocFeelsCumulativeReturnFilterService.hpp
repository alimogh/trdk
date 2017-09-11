/*******************************************************************************
 *   Created: 2017/09/05 18:46:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"
#include "DocFeelsCumulativeReturnService.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API CumulativeReturnFilterService
    : public Service {
 public:
  typedef CumulativeReturnService::Point::R R;
  struct Point {
    struct Value {
      size_t number;
      //! Signal of CR(RT.X) which was the highest.
      CumulativeReturnService::Point::R r;
      //! Select CR(RT.X) which was the highest 30-CR(RT.X).
      trdk::Lib::Double cr;
      //! SMA(CR(RT.X)).
      trdk::Lib::Double crSma;
      //! Select highest 30-CR(RT.X).
      trdk::Lib::Double crPeriod;
      R signal;
      R nprtf;
    };
    boost::posix_time::ptime time;
    trdk::Price source;
    trdk::Price change;
    trdk::Lib::Double tOld;
    trdk::Lib::Double tNew;
    Value bestTOldValues;
    Value bestTNewValues;
  };

 public:
  explicit CumulativeReturnFilterService(Context &,
                                         const std::string &instanceName,
                                         const Lib::IniSectionRef &);
  virtual ~CumulativeReturnFilterService() override;

 public:
  virtual void OnServiceStart(const trdk::Service &);

  virtual boost::posix_time::ptime GetLastDataTime() const override;

  virtual bool IsEmpty() const;

  virtual const Point &GetLastPoint() const;

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