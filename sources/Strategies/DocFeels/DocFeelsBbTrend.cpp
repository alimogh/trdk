/*******************************************************************************
 *   Created: 2017/08/30 10:04:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsBbTrend.hpp"
#include "Core/Position.hpp"
#include "Services/BollingerBandsService.hpp"
#include "DocFeelsPositionController.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies::DocFeels;

BbTrend::BbTrend(const Lib::IniSectionRef &) {}

bool BbTrend::OnServiceStart(const Service &service) {
  return SetService<BollingerBandsService>(service);
}

Price BbTrend::GetUpperControlValue() const {
  const auto &bb = GetService<BollingerBandsService>();
  Assert(!bb.IsEmpty());
  return bb.GetLastPoint().high;
}
Price BbTrend::GetLowerControlValue() const {
  const auto &bb = GetService<BollingerBandsService>();
  Assert(!bb.IsEmpty());
  return bb.GetLastPoint().low;
}
