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

#include "Core/Service.hpp"

namespace trdk {
namespace Tests {

class MockService : public trdk::Service {
 public:
  MockService();
  virtual ~MockService();

 public:
  MOCK_CONST_METHOD0(GetLastDataTime, const boost::posix_time::ptime &());
};
}
}
