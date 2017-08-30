/*******************************************************************************
 *   Created: 2017/08/20 14:15:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "TradingLib/SourcesSynchronizer.hpp"
#include "Core/Strategy.hpp"
#include "Services/BarService.hpp"
#include "Api.h"
#include "DocFeelsPositionController.hpp"
#include "DocFeelsTrend.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

 public:
  explicit Strategy(trdk::Context &,
                    const std::string &instanceName,
                    const trdk::Lib::IniSectionRef &,
                    const boost::shared_ptr<Trend> &);
  virtual ~Strategy() override = default;

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnServiceStart(const trdk::Service &) override;
  virtual void OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  virtual void OnPostionsCloseRequest() override;

 private:
  void CheckSignal(const Lib::TimeMeasurement::Milestones &);

 private:
  const boost::shared_ptr<Trend> m_trend;
  PositionController m_positionController;
  Security *m_security;
  TradingLib::SourcesSynchronizer m_sourcesSync;
  const Services::BarService *m_barService;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}