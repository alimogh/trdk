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

namespace trdk {
namespace Interaction {
namespace Rest {

struct Settings {
  explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &log) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &) {}

  void Validate() {}
};
}
}
}
