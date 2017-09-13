/*******************************************************************************
 *   Created: 2017/09/12 00:51:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsCtsTrend.hpp"

using namespace trdk;
using namespace trdk::Strategies::DocFeels;

CtsTrend::CtsTrend(const Lib::IniSectionRef &) {}

bool CtsTrend::Update(intmax_t confluence) {
  if (confluence > 0) {
    return SetIsRising(true);
  } else if (confluence < 0) {
    return SetIsRising(false);
  } else {
    return false;
  }
}