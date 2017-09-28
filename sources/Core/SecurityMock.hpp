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

#include "ContextDummy.hpp"
#include "MarketDataSourceDummy.hpp"
#include "Security.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class Security : public trdk::Security {
 public:
  Security(const char *symbol = "TEST_SCALE2*/USD:NYMEX:FUT")
      : trdk::Security(Dummies::Context::GetInstance(),
                       trdk::Lib::Symbol(symbol),
                       Dummies::MarketDataSource::GetInstance(),
                       SupportedLevel1Types()),
        m_symbol(nullptr) {}

  virtual ~Security() override = default;

 public:
  //! @todo Fix when Google Test will support noexcept.
  void SetSymbolToMock(const trdk::Lib::Symbol &symbol) { m_symbol = &symbol; }
  //! @todo Fix when Google Test will support noexcept.
  const trdk::Lib::Symbol &GetSymbol() const noexcept override {
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
  const trdk::Lib::Symbol *m_symbol;
};
}
}
}