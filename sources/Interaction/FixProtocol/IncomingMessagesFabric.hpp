/*******************************************************************************
 *   Created: 2017/10/01 06:02:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/
#pragma once

#include "Message.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {
namespace Incoming {

class Factory {
 public:
  typedef Message::Iterator Iterator;

 private:
  Factory();
  ~Factory();

 public:
  static std::unique_ptr<Message> Create(const Iterator &begin,
                                         const Iterator &end,
                                         const Policy &);
};
}
}
}
}
