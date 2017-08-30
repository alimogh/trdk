/*******************************************************************************
 *   Created: 2017/08/26 23:52:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsSmaTrend.hpp"
#include "Core/Position.hpp"
#include "Services/MovingAverageService.hpp"
#include "DocFeelsPositionController.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies::DocFeels;

SmaTrend::SmaTrend(const Lib::IniSectionRef &) {}

bool SmaTrend::OnServiceStart(const Service &service) {
  return SetService<MovingAverageService>(service);
}

Price SmaTrend::GetUpperControlValue() const { return GetControlValue(); }
Price SmaTrend::GetLowerControlValue() const { return GetControlValue(); }

Price SmaTrend::GetControlValue() const {
  const auto &ma = GetService<MovingAverageService>();
  Assert(!ma.IsEmpty());
  return ma.GetLastPoint().value;
}