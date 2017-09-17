/*******************************************************************************
 *   Created: 2017/09/17 20:57:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "TradingLib/SourcesSynchronizer.hpp"
#include "DocFeelsStrategy.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API PureStrategy : public DocFeels::Strategy {
 public:
  typedef DocFeels::Strategy Base;

 public:
  explicit PureStrategy(trdk::Context &,
                        const std::string &instanceName,
                        const trdk::Lib::IniSectionRef &,
                        const boost::shared_ptr<Trend> &);
  virtual ~PureStrategy() override = default;

 protected:
  Trend &GetTrend() { return GetTypedTrend<Trend>(); }
  const Trend &GetTrend() const { return GetTypedTrend<Trend>(); }

 protected:
  virtual void OnServiceStart(const trdk::Service &) override;
  virtual void OnServiceDataUpdate(
      const Service &, const Lib::TimeMeasurement::Milestones &) override;
  virtual void CheckSignal(const Lib::TimeMeasurement::Milestones &) override;

 private:
  TradingLib::SourcesSynchronizer m_sourcesSync;
};
}
}
}