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
  struct Point {
    struct Value {
      size_t number;
      CumulativeReturnService::Point::R r;
      trdk::Lib::Double rt;
      trdk::Lib::Double cr;
      trdk::Lib::Double crPeriod;
    };
    boost::posix_time::ptime time;
    trdk::Price source;
    trdk::Price change;
    trdk::Lib::Double tOld;
    trdk::Lib::Double tNew;
    std::vector<Value> bestTOldValues;
    std::vector<Value> bestTNewValues;
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