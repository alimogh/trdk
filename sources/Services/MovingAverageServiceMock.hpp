/*******************************************************************************
 *   Created: 2017/07/23 02:11:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "MovingAverageService.hpp"

namespace trdk {
namespace Tests {

class MovingAverageServiceMock : public trdk::Services::MovingAverageService {
 public:
  MovingAverageServiceMock();
  virtual ~MovingAverageServiceMock() override;

 public:
  MOCK_CONST_METHOD0(IsEmpty, bool());
  MOCK_CONST_METHOD0(GetLastPoint, const Point &());
};
}
}
