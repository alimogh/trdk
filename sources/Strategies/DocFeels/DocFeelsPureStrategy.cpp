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
#include "DocFeelsPureStrategy.hpp"
#include "Core/TradingLog.hpp"
#include "DocFeelsBbTrend.hpp"
#include "DocFeelsMaTrend.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;

PureStrategy::PureStrategy(Context &context,
                           const std::string &instanceName,
                           const IniSectionRef &conf,
                           const boost::shared_ptr<Trend> &trend)
    : Base(context,
           "{83511675-2A68-4971-9F90-5B96911E0120}",
           "DocFeelsPure",
           instanceName,
           conf,
           trend) {}

void PureStrategy::OnServiceStart(const Service &service) {
  if (m_sourcesSync.GetSize() >= 2) {
    GetLog().Error(
        "Strategy works with one bar service and one control service, but "
        "set more (\"%1%\").",
        service);
    throw Exception("Too many services set for strategy");
  }
  if (!GetTrend().OnServiceStart(service)) {
    Base::OnServiceStart(service);
  }
  m_sourcesSync.Add(service);
}

void PureStrategy::OnServiceDataUpdate(const Service &service,
                                       const Milestones &delayMeasurement) {
  if (m_sourcesSync.GetSize() != 2) {
    throw Exception("One or more of required services are not set");
  } else if (!m_sourcesSync.Sync(service)) {
    return;
  }
  Base::OnServiceDataUpdate(service, delayMeasurement);
}

void PureStrategy::CheckSignal(const Milestones &delayMeasurement) {
  const auto price =
      GetSecurity().DescalePrice(GetBarService().GetLastBar().closeTradePrice);
  const bool isTrendChanged = GetTrend().Update(price);

  GetTradingLog().Write(
      "trend\t%1%\t%2%\tprice=%3$.8f\tcontrol=%4$.8f-%5$.8f",
      [&](TradingRecord &record) {
        record % (isTrendChanged ? "CHANGED" : "not-changed")  // 1
            % (GetTrend().IsRising()
                   ? "rising"
                   : GetTrend().IsFalling() ? "falling" : "-")  // 2
            % price                                             // 3
            % GetTrend().GetLowerControlValue()                 // 4
            % GetTrend().GetUpperControlValue();                // 5
      });

  if (isTrendChanged) {
    OnSignal(delayMeasurement);
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::Strategy> CreatePureSmaStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  return boost::make_shared<PureStrategy>(context, instanceName, conf,
                                          boost::make_shared<MaTrend>());
}

boost::shared_ptr<trdk::Strategy> CreatePureBbStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  return boost::make_shared<PureStrategy>(context, instanceName, conf,
                                          boost::make_shared<BbTrend>());
}

////////////////////////////////////////////////////////////////////////////////
