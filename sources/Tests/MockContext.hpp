/*******************************************************************************
 *   Created: 2016/02/07 02:13:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/RiskControl.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace trdk { namespace Tests {

	class MockContext : public Context {

	public:

		MockContext();
		virtual ~MockContext();

	public:

		MOCK_CONST_METHOD0(SyncDispatching, std::unique_ptr<DispatchingLock>());
		
		virtual RiskControl & GetRiskControl(const trdk::TradingMode &);
		virtual const RiskControl & GetRiskControl(
				const trdk::TradingMode &)
				const;

		MOCK_CONST_METHOD0(GetDropCopy, DropCopy *());

		MOCK_CONST_METHOD0(
			GetExpirationCalendar,
			const trdk::Lib::ExpirationCalendar &());

		MOCK_CONST_METHOD0(GetNumberOfMarketDataSources, size_t());

		MOCK_CONST_METHOD1(
			GetMarketDataSource,
			const trdk::MarketDataSource &(size_t index));

		MOCK_METHOD1(
			GetMarketDataSource,
			trdk::MarketDataSource &(size_t index));
		MOCK_CONST_METHOD1(
			ForEachMarketDataSource,
			void(const boost::function<bool(const trdk::MarketDataSource &)> &));
		MOCK_METHOD1(
			ForEachMarketDataSource,
			void(const boost::function<bool(trdk::MarketDataSource &)> &));

		MOCK_CONST_METHOD0(GetNumberOfTradingSystems,  size_t ());
		MOCK_CONST_METHOD2(
			GetTradingSystem,
			const trdk::TradingSystem &(size_t index, const trdk::TradingMode &));
		MOCK_METHOD2(
			GetTradingSystem,
			trdk::TradingSystem &(size_t index, const trdk::TradingMode &));

	};


} }
