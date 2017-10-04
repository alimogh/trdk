/*******************************************************************************
 *   Created: 2016/02/07 02:13:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Context.hpp"
#include "MarketDataSource.hpp"
#include "RiskControl.hpp"
#include "TradingSystem.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class Context : public trdk::Context {
 public:
  Context();
  virtual ~Context() override = default;

 public:
  MOCK_CONST_METHOD0(SyncDispatching, std::unique_ptr<DispatchingLock>());

  virtual RiskControl &GetRiskControl(const trdk::TradingMode &);
  virtual const RiskControl &GetRiskControl(const trdk::TradingMode &) const;

  MOCK_CONST_METHOD0(GetDropCopy, DropCopy *());

  MOCK_CONST_METHOD0(GetExpirationCalendar,
                     const trdk::Lib::ExpirationCalendar &());
  MOCK_CONST_METHOD0(HasExpirationCalendar, bool());

  MOCK_CONST_METHOD0(GetNumberOfMarketDataSources, size_t());

  MOCK_CONST_METHOD1(GetMarketDataSource,
                     const trdk::MarketDataSource &(size_t index));

  MOCK_METHOD1(GetMarketDataSource, trdk::MarketDataSource &(size_t index));
  MOCK_CONST_METHOD1(
      ForEachMarketDataSource,
      void(const boost::function<void(const trdk::MarketDataSource &)> &));
  MOCK_METHOD1(ForEachMarketDataSource,
               void(const boost::function<void(trdk::MarketDataSource &)> &));

  MOCK_CONST_METHOD0(GetNumberOfTradingSystems, size_t());
  MOCK_CONST_METHOD2(GetTradingSystem,
                     const trdk::TradingSystem &(size_t index,
                                                 const trdk::TradingMode &));
  MOCK_METHOD2(GetTradingSystem,
               trdk::TradingSystem &(size_t index, const trdk::TradingMode &));

  MOCK_CONST_METHOD0(GetCurrentTime, boost::posix_time::ptime());
};
}
}
}
