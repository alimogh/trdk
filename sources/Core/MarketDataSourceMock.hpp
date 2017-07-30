/*******************************************************************************
 *   Created: 2016/02/07 12:10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "MarketDataSource.hpp"
#include "Security.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class MarketDataSource : public trdk::MarketDataSource {
 public:
  MarketDataSource();
  virtual ~MarketDataSource() override = default;

 public:
  MOCK_METHOD1(Connect, void(const trdk::Lib::IniSectionRef &));

  MOCK_METHOD0(SubscribeToSecurities, void());

 protected:
  MOCK_METHOD1(CreateNewSecurityObject,
               trdk::Security &(const trdk::Lib::Symbol &));
};
}
}
}
