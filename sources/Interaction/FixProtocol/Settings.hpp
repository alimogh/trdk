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
  std::string username;
  std::string password;
  std::string senderCompId;
  std::string targetCompId;
  std::string senderSubId;
  std::unique_ptr<Policy> policy;

  Settings(const Lib::IniSectionRef &);
  ~Settings();
  void Log(Module::Log &) const;
  void Validate() const;
};
}
}
}