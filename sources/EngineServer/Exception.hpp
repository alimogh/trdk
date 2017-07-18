/**************************************************************************
 *   Created: 2015/04/11 13:52:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {
namespace EngineServer {

class Exception : public trdk::Lib::Exception {
 public:
  explicit Exception(const char *what) throw() : Lib::Exception(what) {}
};
}
}
