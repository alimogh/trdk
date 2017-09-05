/*******************************************************************************
 *   Created: 2017/08/20 14:18:12
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
#include "DocFeelsCumulativeReturnFilterService.hpp"
#include "DocFeelsCumulativeReturnService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;
namespace df = trdk::Strategies::DocFeels;

df::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf,
                       const boost::shared_ptr<Trend> &trend)
    : Base(context,
           "{afbc3244-cc9c-4aa0-bc94-223f9232f043}",
           "DocFeels",
           instanceName,
           conf),
      m_trend(trend),
      m_positionController(*this, *m_trend),
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
  if (dynamic_cast<const CumulativeReturnService *>(&service) ||
      dynamic_cast<const CumulativeReturnFilterService *>(&service)) {
    return;
  }
  if (m_sourcesSync.GetSize() >= 2) {
    GetLog().Error(
        "Strategy works with one bar service and one control service, but "
        "set more (\"%1%\").",
        service);
    throw Exception("Too many services set for strategy");
  }

  const auto *barService = dynamic_cast<const Services::BarService *>(&service);
  if (barService) {
    if (m_barService) {
      throw Exception("Strategy uses one bar-service, but configured more");
    }
    m_barService = barService;
  } else if (!m_trend->OnServiceStart(service)) {
    GetLog().Error("Service \"%1%\" has unknown type.", service);
    throw Exception("Service with unknown type is set");
  }
  m_sourcesSync.Add(service);
  GetLog().Info("Using \"%1%\" as %2%-service...",
                barService ? "bar" : "control",  // 1
                service);                        // 2
}

void df::Strategy::OnServiceDataUpdate(const Service &service,
                                       const Milestones &delayMeasurement) {
  if (m_sourcesSync.GetSize() != 2 || !m_security) {
    throw Exception("One or more of required services are not set");
  } else if (!m_sourcesSync.Sync(service)) {
    return;
  }
  CheckSignal(delayMeasurement);
}

void df::Strategy::OnPositionUpdate(Position &position) {
  m_positionController.OnPositionUpdate(position);
}

void df::Strategy::OnPostionsCloseRequest() {
  m_positionController.OnPostionsCloseRequest();
}

void df::Strategy::CheckSignal(const Milestones &delayMeasurement) {
  Assert(m_security);
  Assert(m_barService);
  Assert(!m_barService->IsEmpty());
  AssertEq(2, m_sourcesSync.GetSize());

  const auto price =
      m_security->DescalePrice(m_barService->GetLastBar().closeTradePrice);
  const bool isTrendChanged = m_trend->Update(price);

  GetTradingLog().Write(
      "trend\t%1%\t%2%\tprice=%3$.8f\tcontrol=%4$.8f-%5$.8f",
      [&](TradingRecord &record) {
        record % (isTrendChanged ? "changed" : "not-changed")  // 1
            %
            (m_trend->IsRising() ? "rising"
                                 : m_trend->IsFalling() ? "falling" : "-")  // 2
            % price                                                         // 3
            % m_trend->GetLowerControlValue()                               // 4
            % m_trend->GetUpperControlValue();                              // 5
      });

  if (isTrendChanged) {
    m_positionController.OnSignal(*m_security, delayMeasurement);
  }
}
