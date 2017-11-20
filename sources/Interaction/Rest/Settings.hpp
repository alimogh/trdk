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
  boost::posix_time::time_duration pullingInterval;

  explicit Settings(const Lib::IniSectionRef &conf, ModuleEventsLog &log)
      : pullingInterval(boost::posix_time::milliseconds(
            conf.ReadTypedKey<long>("pulling_interval_milliseconds"))) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("Pulling interval: %1%.", pullingInterval);
  }

  void Validate() {}
};
}
}
}
