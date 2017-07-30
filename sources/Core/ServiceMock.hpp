/**************************************************************************
 *   Created: 2017/01/09 00:16:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Service.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class Service : public trdk::Service {
 public:
  Service();
  virtual ~Service() override = default;

 public:
  MOCK_CONST_METHOD0(GetLastDataTime, const boost::posix_time::ptime &());
};
}
}
}