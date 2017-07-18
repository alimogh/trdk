/**************************************************************************
 *   Created: 2016/09/12 01:39:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"
#include "DummyContext.hpp"
#include "DummyMarketDataSource.hpp"

namespace trdk {
namespace Tests {

class MockSecurity : public trdk::Security {
 public:
  MockSecurity(const char *symbol = "TEST_SCALE2*/USD:NYMEX:FUT")
      : Security(DummyContext::GetInstance(),
                 trdk::Lib::Symbol(symbol),
                 DummyMarketDataSource::GetInstance(),
                 SupportedLevel1Types()) {}

  virtual ~MockSecurity() {}

 public:
  MOCK_CONST_METHOD0(IsOnline, bool());
  MOCK_CONST_METHOD0(GetExpiration, const trdk::Lib::ContractExpiration &());
};
}
}
