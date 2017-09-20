/*******************************************************************************
 *   Created: 2017/09/20 18:11:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Module.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {
struct Settings {
  std::string host;
  size_t port;

  Settings(const Lib::IniSectionRef &);
  void Log(Module::Log &) const;
  void Validate() const;
};
}
}
}
