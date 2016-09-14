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
#include "Services/ContinuousContractBarService.hpp"
#include "Services/BarService.hpp"
#include "MockContext.hpp"
#include "MockMarketDataSource.hpp"
#include "MockDropCopy.hpp"
#include "MockSecurity.hpp"

using namespace trdk::Tests;
using namespace testing;

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

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

	class TestBarService : public svc::BarService {
	public:
		explicit TestBarService(
				trdk::Context &context,
				const std::string &tag,
				const lib::IniSectionRef &conf)
			: BarService(context, tag, conf) {
			//...//
		}
	public:
		using BarService::OnSecurityStart;
		using BarService::OnLevel1Tick;
		using BarService::OnNewTrade;
		using BarService::OnSecurityServiceEvent;
	};

	class TestContinuousContractBarService
		: public svc::ContinuousContractBarService {
	public:
		typedef svc::ContinuousContractBarService Base;
	public:
		explicit TestContinuousContractBarService(
				trdk::Context &context,
				const std::string &tag,
				const lib::IniSectionRef &conf)
			: Base(context, tag, conf) {
			//...//
		}
	public:
		using Base::OnSecurityStart;
		using Base::OnNewTrade;
		using Base::OnSecurityServiceEvent;
	};

}

////////////////////////////////////////////////////////////////////////////////

TEST(BarServiceTest, ByNumberOfTicks) {
	
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
		true,
		trdk::Security::SupportedLevel1Types());
	
	TestBarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));
	EXPECT_EQ(service.OnSecurityStart(security), pt::not_a_date_time);

	const size_t numberOfSets = 39;
	const Source &source = GetSource(numberOfSets);

	MockDropCopy dropCopy;
	EXPECT_CALL(
		dropCopy,
		CopyBar(
			_,
			AllOf(Ge(source.front().time), Le(source.back().time)),
			Matcher<size_t>(10),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price)),
			AllOf(Ge(source.front().price), Le(source.back().price))))
		.Times(int(source.size()));
	EXPECT_CALL(context, GetDropCopy())
		.Times(int(source.size()))
		.WillRepeatedly(Return(&dropCopy));

	double volume = 0;
	for (size_t i = 0, tradesCount = 0; i < source.size(); ++i) {
			
		const auto &tick = source[i];
			
		ASSERT_THROW(
			service.OnLevel1Tick(
				security,
				tick.time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					tick.price)),
			TestBarService::MethodDoesNotSupportBySettings);
		ASSERT_THROW(
			service.OnLevel1Tick(
				security,
				tick.time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_QTY>(
					tick.qty)),
			TestBarService::MethodDoesNotSupportBySettings);

		++tradesCount;
		volume += tick.qty;

		if (service.OnNewTrade(security, tick.time, tick.price, tick.qty)) {
			
			ASSERT_EQ(10, tradesCount) << "i = " << i << ";";
			ASSERT_EQ((i + 1) / 10, service.GetSize());

			ASSERT_EQ(0, service.GetLastBar().openBidPrice);
			ASSERT_EQ(0, service.GetLastBar().openAskPrice);
			ASSERT_EQ(0, service.GetLastBar().closeBidPrice);
			ASSERT_EQ(0, service.GetLastBar().closeAskPrice);
			ASSERT_EQ(0, service.GetLastBar().minBidPrice);
			ASSERT_EQ(0, service.GetLastBar().maxAskPrice);

			// For "counted bar" time of bar is time of last tick.
			ASSERT_EQ(tick.time, service.GetLastBar().time);
			ASSERT_EQ(
				source[i + 1 - tradesCount].price,
				service.GetLastBar().openTradePrice);
			ASSERT_EQ(tick.price, service.GetLastBar().closeTradePrice);
			ASSERT_EQ(tick.price, service.GetLastBar().highTradePrice);
			ASSERT_EQ(
				source[i + 1 - tradesCount].price,
				service.GetLastBar().lowTradePrice);
			ASSERT_DOUBLE_EQ(volume, service.GetLastBar().tradingVolume);

			tradesCount = 0;
			volume = 0;

		} else {

			ASSERT_GT(10, tradesCount) << "i = " << i << ";";
			ASSERT_GE(i + 1, tradesCount);
			ASSERT_EQ(i >= 10, service.GetSize() > 0)
				<< "i = " << i << "; service.GetSize() = " << service.GetSize();

			if (service.GetSize() > 0) {

				const auto &prevTick = source[i - tradesCount];

				// For "counted bar" time of bar is time of last tick.
				ASSERT_EQ(prevTick.time, service.GetLastBar().time);
				ASSERT_EQ(
					source[i + 1 - tradesCount - 10].price,
					service.GetLastBar().openTradePrice);
				ASSERT_EQ(prevTick.price, service.GetLastBar().closeTradePrice);
				ASSERT_EQ(prevTick.price, service.GetLastBar().highTradePrice);
				ASSERT_EQ(
					source[i + 1 - tradesCount - 10].price,
					service.GetLastBar().lowTradePrice);
			
			} else {
				EXPECT_THROW(
					service.GetLastBar(),
					TestBarService::BarDoesNotExistError);
			}

		}

	}

}

TEST(BarServiceTest, ByNumberOfTicksWithSessionOpenClose) {
	
	const lib::IniString settings(
		"[Section]\n"
		"size = 10000 ticks\n"
		"log = none");
	
	MockContext context;
	MockMarketDataSource marketDataSource(0, context, std::string("0"));
	const trdk::Security security(
		context,
		lib::Symbol("TEST_SCALE2/USD::STK"),
		marketDataSource,
		true,
		true,
		trdk::Security::SupportedLevel1Types());
	TestBarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	MockDropCopy dropCopy;
	EXPECT_CALL(
		dropCopy,
		CopyBar(
			_,
			_,
			Matcher<size_t>(10000),
			AllOf(Ge(1), Le(4)),
			AllOf(Ge(1), Le(4)),
			AllOf(Ge(1), Le(4)),
			AllOf(Ge(1), Le(4))))
		.Times(6);
	EXPECT_CALL(context, GetDropCopy())
		.Times(6)
		.WillRepeatedly(Return(&dropCopy));

	EXPECT_EQ(service.OnSecurityStart(security), pt::not_a_date_time);
	EXPECT_FALSE(
		service.OnSecurityServiceEvent(
			security,
			trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));

	EXPECT_FALSE(
		service.OnSecurityServiceEvent(
			security,
			trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));
	EXPECT_EQ(0, service.GetSize());
	EXPECT_THROW(service.GetLastBar(), TestBarService::BarDoesNotExistError);

	EXPECT_FALSE(
		service.OnNewTrade(
			security,
			pt::microsec_clock::local_time(),
			1,
			1));
	EXPECT_EQ(0, service.GetSize());
	EXPECT_THROW(service.GetLastBar(), TestBarService::BarDoesNotExistError);
	EXPECT_FALSE(
		service.OnNewTrade(
			security,
			pt::microsec_clock::local_time(),
			2,
			2));
	EXPECT_EQ(0, service.GetSize());
	EXPECT_THROW(service.GetLastBar(), TestBarService::BarDoesNotExistError);

	EXPECT_TRUE(
		service.OnSecurityServiceEvent(
			security,
			trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));
	EXPECT_EQ(1, service.GetSize());
	EXPECT_EQ(1, service.GetLastBar().openTradePrice);
	EXPECT_EQ(2, service.GetLastBar().closeTradePrice);
	EXPECT_EQ(2, service.GetLastBar().highTradePrice);
	EXPECT_EQ(1, service.GetLastBar().lowTradePrice);

	EXPECT_FALSE(
		service.OnNewTrade(
			security,
			pt::microsec_clock::local_time(),
			3,
			3));
	EXPECT_EQ(1, service.GetSize());

	EXPECT_TRUE(
		service.OnSecurityServiceEvent(
			security,
			trdk::Security::SERVICE_EVENT_TRADING_SESSION_OPENED));
	EXPECT_EQ(2, service.GetSize());
	EXPECT_EQ(3, service.GetLastBar().openTradePrice);
	EXPECT_EQ(3, service.GetLastBar().closeTradePrice);
	EXPECT_EQ(3, service.GetLastBar().highTradePrice);
	EXPECT_EQ(3, service.GetLastBar().lowTradePrice);

	EXPECT_FALSE(
		service.OnNewTrade(
			security,
			pt::microsec_clock::local_time(),
			4,
			4));
	EXPECT_EQ(2, service.GetSize());
	EXPECT_EQ(3, service.GetLastBar().openTradePrice);
	EXPECT_EQ(3, service.GetLastBar().closeTradePrice);
	EXPECT_EQ(3, service.GetLastBar().highTradePrice);
	EXPECT_EQ(3, service.GetLastBar().lowTradePrice);

}

TEST(BarServiceTest, ClosingCountedBarByLastBarTick) {

	const lib::IniString settings(
		"[Section]\n"
		"size = 2 ticks\n"
		"log = none");

	MockContext context;
	EXPECT_CALL(context, GetDropCopy()).WillRepeatedly(Return(nullptr));

	MockSecurity security;
	TestBarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));
	
	EXPECT_EQ(service.OnSecurityStart(security), pt::not_a_date_time);

	const auto &now = pt::microsec_clock::local_time();
	EXPECT_FALSE(service.OnNewTrade(security,  now, 1, 3));
	EXPECT_EQ(0, service.GetSize());
	EXPECT_TRUE(service.OnNewTrade(security,  now, 1, 3));
	EXPECT_EQ(1, service.GetSize());
	EXPECT_FALSE(service.OnNewTrade(security, now, 1, 3));
	EXPECT_EQ(1, service.GetSize());
	EXPECT_TRUE(service.OnNewTrade(security,  now, 1, 3));
	EXPECT_EQ(2, service.GetSize());

}

TEST(BarServiceTest, ContinuousContract) {

	const lib::IniString settings(
		"[Section]\n"
		"size = 4 ticks\n"
		"log = none");

	MockContext context;
	EXPECT_CALL(context, GetDropCopy()).WillRepeatedly(Return(nullptr));
	
	MockSecurity security;

	TestContinuousContractBarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	EXPECT_EQ(service.OnSecurityStart(security), pt::not_a_date_time);

	const double source[12][8] = {
		{100,	24,	2500,	48,	133.3333333,	22.90909091,	2592.592593,	49.23076923},
		{200,	23,	2600,	47,	266.6666667,	21.95454545,	2696.296296,	48.20512821},
		{300,	22,	2700,	39,	400,			21,				2800,			40},
		{400,	21,	2800,	40,	466.6666667,	19.89473684,	3577.777778,	40.95238095},
		{500,	20,	2900,	41,	583.3333333,	18.94736842,	3705.555556,	41.97619048},
		{600,	19,	3600,	42,	700,			18,				4600,			43},
		{700,	18,	4600,	43,	777.7777778,	16.875,			5205.263158,	43.95555556},
		{800,	17,	5600,	44,	888.8888889,	15.9375,		6336.842105,	44.97777778},
		{900,	16,	7600,	45,	1000,			15,				8600,			46},
		{1000,	15,	8600,	46,	1000,			15,				8600,			46},
		{1100,	14,	9600,	47,	1100,			14,				9600,			47},
		{1200,	13,	7600,	48,	1200,			13,				7600,			48},
	};

	EXPECT_CALL(security, GetExpiration())
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 6, 12)))) // 0
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 6, 12)))) // 1
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 6, 12)))) // 2
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 7, 12)))) // 3
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 7, 12)))) // 4
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 7, 12)))) // 5
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 8, 12)))) // 6
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 8, 12)))) // 7
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 8, 12)))) // 8
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 9, 12)))) // 9
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 9, 12)))) // 10
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 9, 12)))) // 11
		.WillOnce(Return(lib::ContractExpiration(gr::date(2016, 9, 12)))); // building
	EXPECT_CALL(security, IsOnline())
		.WillOnce(Return(false)) // 0
		.WillOnce(Return(false)) // 1
		.WillOnce(Return(false)) // 2
		.WillOnce(Return(false)) // 3
		.WillOnce(Return(false)) // 4
		.WillOnce(Return(false)) // 5
		.WillOnce(Return(false)) // 6
		.WillOnce(Return(false)) // 7
		.WillOnce(Return(false)) // 8
		.WillOnce(Return(false)) // 9
		.WillOnce(Return(false)) // 10
		.WillOnce(Return(true)); // 11

	for (size_t i = 0; i < 12; ++i) {

		pt::ptime now;
		if (i <= 2) {
			now = pt::ptime(gr::date(2016, 6, 10));
		} else if (i <= 5) {
			now = pt::ptime(gr::date(2016, 7, 10));
		} else if (i <= 8) {
			now = pt::ptime(gr::date(2016, 8, 10));
		} else {
			now = pt::ptime(gr::date(2016, 9, 10));
		}

		service.OnNewTrade(
			security, 
			now,
			trdk::ScaledPrice(source[i][0] * 100),
			3);
		service.OnNewTrade(
			security, 
			now,
			trdk::ScaledPrice(source[i][1] * 100),
			3);
		service.OnNewTrade(
			security, 
			now,
			trdk::ScaledPrice(source[i][2] * 100),
			3);
		service.OnNewTrade(
			security, 
			now,
			trdk::ScaledPrice(source[i][3] * 100),
			3);

	}

	EXPECT_TRUE(
		service.OnSecurityServiceEvent(
			security,
			trdk::Security::SERVICE_EVENT_ONLINE));

	ASSERT_EQ(_countof(source), service.GetSize());

	for (size_t i = 0; i < 12; ++i) {
		const auto &bar = service.GetBar(i);
		EXPECT_EQ(0, bar.maxAskPrice) << i;
		EXPECT_EQ(0, bar.openAskPrice) << i;
		EXPECT_EQ(0, bar.closeAskPrice) << i;
		EXPECT_EQ(0, bar.minBidPrice) << i;
		EXPECT_EQ(0, bar.openBidPrice) << i;
		EXPECT_EQ(0, bar.closeBidPrice) << i;
		EXPECT_EQ(trdk::ScaledPrice(source[i][4] * 100), bar.openTradePrice)
			<< i;
		EXPECT_EQ(trdk::ScaledPrice(source[i][5] * 100), bar.lowTradePrice)
			<< i;
		EXPECT_EQ(trdk::ScaledPrice(source[i][6] * 100), bar.highTradePrice)
			<< i;
		EXPECT_EQ(trdk::ScaledPrice(source[i][7] * 100), bar.closeTradePrice)
			<< i;
	}

}
