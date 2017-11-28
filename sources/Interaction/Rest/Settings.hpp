/*******************************************************************************
 *   Created: 2017/11/16 13:21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "PullingSettings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

struct Settings {
  PullingSetttings pullingSetttings;

  explicit Settings(const Lib::IniSectionRef &conf, ModuleEventsLog &log)
      : pullingSetttings(conf) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) { pullingSetttings.Log(log); }

  void Validate() {}
};
}
}
}
