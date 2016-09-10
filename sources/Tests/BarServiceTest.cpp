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
#include "MockDropCopy.hpp"
#include "Core/Security.hpp"

using namespace trdk::Tests;
using namespace testing;

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct Tick {
	
		pt::ptime time;
		trdk::ScaledPrice price;
		trdk::Qty qty;

		explicit Tick(
				const pt::ptime &time,
				const trdk::ScaledPrice &price,
				const trdk::Qty &qty)
			: time(time)
			, price(price)
			, qty(qty) {
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
				i + 1,
				i + 2);
		}

		return result;

	}

}

////////////////////////////////////////////////////////////////////////////////

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
		true,
		trdk::Security::SupportedLevel1Types());
	svc::BarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	const size_t numberOfSets = 39;
	const Source &source = GetSource(numberOfSets);

	MockDropCopy dropCopy;
	EXPECT_CALL(
		dropCopy,
		CopyBar(
			_,
			AllOf(Ge(source.front().time), Le(source.back().time)),
			Matcher<size_t>(AllOf(Ge(0), Lt(source.size()))),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price))))
		.Times(int(source.size()));
	EXPECT_CALL(context, GetDropCopy())
		.Times(int(source.size()))
		.WillRepeatedly(Return(&dropCopy));

	for (size_t i = 0, tradesCount = 0; i < source.size(); ++i) {
			
		const auto &tick = source[i];
			
		ASSERT_THROW(
			service.OnLevel1Tick(
				security,
				tick.time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					tick.price)),
			svc::BarService::MethodDoesNotSupportBySettings);
		ASSERT_THROW(
			service.OnLevel1Tick(
				security,
				tick.time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_QTY>(
					tick.qty)),
			svc::BarService::MethodDoesNotSupportBySettings);

		if (service.OnNewTrade(security, tick.time, tick.price, tick.qty)) {
			ASSERT_EQ(10, tradesCount) << "i = " << i << ";";
			tradesCount = 0;
//! @todo add last actual bar test
			
		} else {
			
			ASSERT_GT(10, tradesCount) << "i = " << i << ";";
			
		}

		++tradesCount;

	}
		
	ASSERT_EQ(4, service.GetSize());

	EXPECT_EQ(0, service.GetLastBar().openBidPrice);
	EXPECT_EQ(0, service.GetLastBar().openAskPrice);
	EXPECT_EQ(0, service.GetLastBar().closeBidPrice);
	EXPECT_EQ(0, service.GetLastBar().closeAskPrice);
	EXPECT_EQ(0, service.GetLastBar().minBidPrice);
	EXPECT_EQ(0, service.GetLastBar().maxAskPrice);
		
	EXPECT_EQ(31, service.GetLastBar().openTradePrice); // prev close
	EXPECT_EQ(39, service.GetLastBar().closeTradePrice);
	EXPECT_EQ(39, service.GetLastBar().highTradePrice);
	EXPECT_EQ(31, service.GetLastBar().lowTradePrice);
	EXPECT_EQ(
		(31 + 1) + (32 + 1) + (33 + 1) + (34 + 1) + (35 + 1) + (36 + 1)
			+ (37 + 1) + (38 + 1) + (39 + 1),
		service.GetLastBar().tradingVolume);


//! @todo add test for all bars in history
//! @todo add test with skipped periods

}
