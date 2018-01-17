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

#include "PollingSettings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

struct Settings {
  PollingSetttings pollingSetttings;

  explicit Settings(const Lib::IniSectionRef &conf, ModuleEventsLog &log)
      : pollingSetttings(conf) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) { pollingSetttings.Log(log); }

  void Validate() {}
};
}
}
}
