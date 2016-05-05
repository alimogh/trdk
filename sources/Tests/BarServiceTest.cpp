/**************************************************************************
 *   Created: 2016/04/11 11:17:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Services/BarService.hpp"
#include "MockContext.hpp"
#include "MockMarketDataSource.hpp"
#include "Core/Security.hpp"

using namespace trdk::Tests;

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;

namespace {

	struct Tick {
	
		pt::ptime time;
		trdk::Level1TickValue value;
	
		explicit Tick(const pt::ptime &time, const trdk::Level1TickValue &value)
			: time(time)
			, value(value) {
			//...//
		}
	
	};
	
	typedef std::vector<Tick> Source;

	Source GetSource(const uint32_t numberOfSets) {

		using namespace trdk;
		
		Source result;
		const auto time = pt::microsec_clock::local_time();
		int timeOffset = 0;
		
		size_t numberOfQtyTicks = 0;
		lib::UseUnused(numberOfQtyTicks);
		for (uint32_t i = 0; i < numberOfSets; ++i) {
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(i + 1));
// 			std::cout
// 				<< ++numberOfQtyTicks
// 				<< " LEVEL1_TICK_LAST_PRICE " << i  << " +  1 = " << (i + 1)
// 				<< std::endl;
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>(i + 2));
// 			std::cout
// 				<< ++numberOfQtyTicks
// 				<< " LEVEL1_TICK_LAST_QTY " << i  << " +  2 = " << (i + 2)
// 				<< std::endl;
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(i + 3));
// 			std::cout
// 				<< "LEVEL1_TICK_BID_PRICE " << i  << " +  3 = " << (i + 3)
// 				<< std::endl;
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(i + 4));
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(i + 5));
// 			std::cout
// 				<< "LEVEL1_TICK_ASK_PRICE " << i  << " +  5 = " << (i + 5)
// 				<< std::endl;
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(i + 6));
			result.emplace_back(
				 time + pt::milliseconds(timeOffset++),
				Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>(i + 7));
		}

		return result;

	}

}

namespace trdk { namespace Tests {

	TEST(BarServiceTest, Level1TickByTick) {
	
		const lib::IniString settings(
			"[Section]\n"
			"size = 10 ticks\n"
			"log = none");
		MockContext context;
		MockMarketDataSource marketDataSource(0, context, std::string("0"));
		const trdk::Security security(
			context,
			lib::Symbol("TEST_SCALE2/USD::STK"),
			marketDataSource,
			true);
		svc::BarService service(
			context,
			"Tag",
			lib::IniSectionRef(settings, "Section"));

		const size_t numberOfSets = 39;
		const Source &source = GetSource(numberOfSets);

		for (size_t i = 0, tradesCount = 0; i < source.size(); ++i) {
			
			const auto &tick = source[i];
			
			if (service.OnLevel1Tick(security, tick.time, tick.value)) {
				ASSERT_EQ(10, tradesCount) << "i = " << i << ";";
				tradesCount = 0;
//! @todo add last actual bar test
			
			} else {
			
				ASSERT_GT(10, tradesCount) << "i = " << i << ";";
			
			}
			
			switch (tick.value.GetType()) {
				case LEVEL1_TICK_LAST_PRICE:
				case LEVEL1_TICK_LAST_QTY:
					++tradesCount;
					break;
			}
		
		}
		
		ASSERT_EQ(8, service.GetSize());

		EXPECT_EQ(34 + 3, service.GetLastBar().openBidPrice);
		// bid is opened bar, so ask-open will be "restored" from prev-bar:
		EXPECT_EQ(33 + 5, service.GetLastBar().openAskPrice);

		EXPECT_EQ(38 + 3, service.GetLastBar().closeBidPrice);
		EXPECT_EQ(38 + 5, service.GetLastBar().closeAskPrice);

		EXPECT_EQ(34 + 3, service.GetLastBar().minBidPrice);
		EXPECT_EQ(38 + 5, service.GetLastBar().maxAskPrice);
		
		EXPECT_EQ(34 + 1, service.GetLastBar().openTradePrice); // prev close
		EXPECT_EQ(38 + 1, service.GetLastBar().closeTradePrice);
		EXPECT_EQ(38 + 1, service.GetLastBar().highTradePrice);
		EXPECT_EQ(34 + 1, service.GetLastBar().lowTradePrice);
		EXPECT_EQ(
			(35 + 2) + (36 + 2) + (37 + 2) + (38 + 2),
			service.GetLastBar().tradingVolume);


//! @todo add test for all bars in history
//! @todo add test with skipped periods

	}

} }
