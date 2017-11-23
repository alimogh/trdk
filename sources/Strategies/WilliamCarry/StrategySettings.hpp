/*******************************************************************************
 *   Created: 2017/10/21 04:58:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

enum { NUMBER_OF_STRATEGIES = 4 };

struct StrategySettings {
  struct Target {
    size_t pips;
    boost::posix_time::time_duration delay;
  };

  bool isEnabled;
  unsigned int lotMultiplier;

  Target target1;
  size_t numberOfStepsToTarget;
  boost::optional<Target> target2;

  size_t stopLoss1;
  boost::optional<Target> stopLoss2;
  boost::optional<Target> stopLoss3;

  std::vector<bool> brokers;
};
}
}
}
