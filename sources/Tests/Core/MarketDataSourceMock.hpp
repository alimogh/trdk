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
#include "Common/ExpirationCalendar.hpp"

namespace trdk {
namespace Tests {
namespace Core {

class MarketDataSourceMock : public MarketDataSource {
 public:
  MarketDataSourceMock();
  virtual ~MarketDataSourceMock() override = default;

 public:
  MOCK_METHOD0(Connect, void());
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
  void SwitchToContract(
      Security &security,
      const Lib::ContractExpiration &&expiration) const override {
    //! @todo Remove this method when Google Test will support movement
    //! semantics.
    SwitchToContract(security, expiration);
  }
};
}  // namespace Core
}  // namespace Tests
}  // namespace trdk
