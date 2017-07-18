/*******************************************************************************
 *   Created: 2016/02/07 12:10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"

namespace trdk {
namespace Tests {

class MockMarketDataSource : public MarketDataSource {
 public:
  MockMarketDataSource();
  virtual ~MockMarketDataSource();

 public:
  MOCK_METHOD1(Connect, void(const trdk::Lib::IniSectionRef &));

  MOCK_METHOD0(SubscribeToSecurities, void());

 protected:
  MOCK_METHOD1(CreateNewSecurityObject,
               trdk::Security &(const trdk::Lib::Symbol &));
};
}
}
