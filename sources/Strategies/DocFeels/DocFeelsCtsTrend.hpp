/*******************************************************************************
 *   Created: 2017/09/12 00:50:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "TradingLib/Trend.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class CtsTrend : public TradingLib::Trend {
 public:
  explicit CtsTrend(const Lib::IniSectionRef &);

 public:
  bool Update(intmax_t confluence);
};
}
}
}
