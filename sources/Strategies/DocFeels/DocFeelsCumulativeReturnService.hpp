/*******************************************************************************
 *   Created: 2017/09/03 14:43:04
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

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API CumulativeReturnService : public Service {
 public:
  struct Point {
    typedef int16_t R;
    struct Branch {
      struct T {
        trdk::Lib::Double rt;
        trdk::Lib::Double cr;
        trdk::Lib::Double crPeriod;
      };
      T tOld;
      T tNew;
      R r;
    };

    boost::posix_time::ptime time;
    trdk::Price source;
    trdk::Price change;
    trdk::Lib::Double tOld;
    trdk::Lib::Double tNew;
    std::vector<Branch> branches;
  };

  class TRDK_STRATEGY_DOCFEELS_API RGenerator : private boost::noncopyable {
   public:
    virtual ~RGenerator() = default;

   public:
    virtual Point::R Generate() const = 0;
    virtual uint32_t GetParam() const { return 0; }
  };

 public:
  explicit CumulativeReturnService(
      Context &,
      const std::string &instanceName,
      const Lib::IniSectionRef &,
      const boost::shared_ptr<RGenerator> & = boost::shared_ptr<RGenerator>());
  virtual ~CumulativeReturnService() override;

 public:
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