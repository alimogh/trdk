/*******************************************************************************
 *   Created: 2017/09/17 20:19:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsStrategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;
namespace tl = trdk::TradingLib;
namespace df = trdk::Strategies::DocFeels;

df::Strategy::Strategy(Context &context,
                       const std::string &typeUuid,
                       const std::string &implementationName,
                       const std::string &instanceName,
                       const IniSectionRef &conf,
                       const boost::shared_ptr<tl::Trend> &trend)
    : Base(context, typeUuid, implementationName, instanceName, conf),
      m_trend(trend),
      m_positionController(
          *this, *m_trend, conf.ReadTypedKey<double>("start_qty")),
      m_security(nullptr),
      m_barService(nullptr) {}

void df::Strategy::OnSecurityStart(Security &security,
                                   Security::Request &request) {
  Base::OnSecurityStart(security, request);
  if (m_security) {
    throw Exception(
        "Strategy can not work with more than one trading security");
  }
  m_security = &security;
  GetLog().Info("Using \"%1%\"...", *m_security);
}

void df::Strategy::OnServiceStart(const Service &service) {
  const auto *barService = dynamic_cast<const Services::BarService *>(&service);
  if (!barService) {
    GetLog().Error("Service \"%1%\" has unknown type.", service);
    throw Exception("Service with unknown type is set");
  } else if (m_barService) {
    throw Exception("Strategy uses one bar-service, but configured more");
  }

  m_barService = barService;
  GetLog().Info("Using \"%1%\" as %2%-service...",
                barService ? "bar" : "control",  // 1
                service);                        // 2
}

void df::Strategy::OnServiceDataUpdate(const Service &,
                                       const Milestones &delayMeasurement) {
  if (!m_barService) {
    throw Exception("One or more of required services are not set");
  }
  CheckSignal(delayMeasurement);
}

void df::Strategy::OnPositionUpdate(Position &position) {
  m_positionController.OnPositionUpdate(position);
}

void df::Strategy::OnPostionsCloseRequest() {
  m_positionController.OnPostionsCloseRequest();
}

void df::Strategy::OnSignal(const Milestones &delayMeasurement) {
  Assert(m_security);
  m_positionController.OnSignal(*m_security, delayMeasurement);
}
