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
#include "ContextDummy.hpp"
#include "MarketDataSourceDummy.hpp"

namespace trdk {
namespace Tests {
namespace Core {

class SecurityMock : public Security {
 public:
  SecurityMock(const char *symbol = "TEST_SCALE2*/USD:NYMEX:Futures")
      : Security(ContextDummy::GetInstance(),
                 Lib::Symbol(symbol),
                 MarketDataSourceDummy::GetInstance(),
                 SupportedLevel1Types()),
        m_symbol(nullptr) {}

  ~SecurityMock() override = default;

  //! @todo Fix when Google Test will support noexcept.
  void SetSymbolToMock(const Lib::Symbol &symbol) { m_symbol = &symbol; }
  //! @todo Fix when Google Test will support noexcept.
  const Lib::Symbol &GetSymbol() const noexcept override {
    if (!m_symbol) {
      AssertFail("Symbol is not set for Security mock-object");
      terminate();
    }
    return *m_symbol;
  }

  MOCK_CONST_METHOD0(IsOnline, bool());
  MOCK_CONST_METHOD0(IsTradingSessionOpened, bool());
  MOCK_CONST_METHOD0(GetExpiration, const trdk::Lib::ContractExpiration &());
  MOCK_CONST_METHOD0(GetLastPrice, trdk::Price());
  MOCK_CONST_METHOD0(GetAskPrice, trdk::Price());
  MOCK_CONST_METHOD0(GetBidPrice, trdk::Price());

 private:
  //! @todo Fix when Google Test will support noexcept.
  const Lib::Symbol *m_symbol;
};
}  // namespace Core
}  // namespace Tests
}  // namespace trdk