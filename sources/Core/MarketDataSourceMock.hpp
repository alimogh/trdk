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
#include "Common/ExpirationCalendar.hpp"

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
  MOCK_CONST_METHOD2(FindContractExpiration,
                     boost::optional<trdk::Lib::ContractExpiration>(
                         const trdk::Lib::Symbol &,
                         const boost::gregorian::date &));
  MOCK_CONST_METHOD2(SwitchToContract,
                     void(trdk::Security &,
                          const trdk::Lib::ContractExpiration &));
  virtual void SwitchToContract(
      trdk::Security &security,
      const trdk::Lib::ContractExpiration &&expiration) const override {
    //! @todo Remove this method when Google Test will support movement
    //! semantics.
    SwitchToContract(security, expiration);
  }
};
}
}
}
