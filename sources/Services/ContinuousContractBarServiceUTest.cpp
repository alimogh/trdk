/**************************************************************************
 *   Created: 2016/09/23 08:40:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContinuousContractBarService.hpp"
#include "Tests/MockContext.hpp"
#include "Tests/MockSecurity.hpp"

using namespace testing;
using namespace trdk::Tests;

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

TEST(ContinuousContractBarServiceTest, General) {

	const lib::IniString settings(
		"[Section]\n"
		"size = 4 ticks\n"
		"log = none");

	MockContext context;
	EXPECT_CALL(context, GetDropCopy()).WillRepeatedly(Return(nullptr));
	
	MockSecurity security;

	svc::ContinuousContractBarService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	{
		trdk::Security::Request request;
		service.OnSecurityStart(security, request);
		EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
		EXPECT_EQ(4, request.GetNumberOfTicks());
	}

	struct Tick {
		bool isCurrent;
		pt::ptime time;
		lib::ContractExpiration expiration;
		double open;
		double low;
		double high;
		double close;
		double adjOpen;
		double adjLow;
		double adjHigh;
		double adjClose;
	};

	const Tick source[] = {
		{true	, pt::ptime(gr::date(2016, 5, 12), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 6, 12)), 100	,	33,	2500,	48,		2046.153846,	675.2307692,	51153.84615,	982.1538462	},
		{true	, pt::ptime(gr::date(2016, 5, 12), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 6, 12)), 200	,	32,	2600,	47,		4092.307692,	654.7692308,	53200,			961.6923077	},
		{true	, pt::ptime(gr::date(2016, 5, 12), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 6, 12)), 300	,	31,	2700,	39,		6138.461538,	634.3076923,	55246.15385,	798			},
		{false	, pt::ptime(gr::date(2016, 6, 12), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 400	,	30,	2800,	67,		0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 6, 12), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 500	,	29,	2900,	225,	0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 6, 12), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 600	,	28,	3000,	798,	0,				0,				0,				0			},
		{true	, pt::ptime(gr::date(2016, 6, 13), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 700	,	27,	3100,	642,	3461.111111,	133.5,			15327.77778,	3174.333333	},
		{true	, pt::ptime(gr::date(2016, 6, 13), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 500	,	26,	3200,	156,	2472.222222,	128.5555556,	15822.22222,	771.3333333	},
		{true	, pt::ptime(gr::date(2016, 6, 13), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 7, 12)), 600	,	25,	3300,	72,		2966.666667,	123.6111111,	16316.66667,	356			},
		{false	, pt::ptime(gr::date(2016, 7, 11), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 800	,	24,	3400,	345,	0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 7, 11), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 900	,	23,	3500,	219,	0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 7, 11), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 1000,	22,	3600,	356,	0,				0,				0,				0			},
		{true	, pt::ptime(gr::date(2016, 7, 14), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 1100,	21,	3700,	123,	344.9561404,	6.585526316,	1160.307018,	38.57236842	},
		{true	, pt::ptime(gr::date(2016, 7, 14), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 1200,	20,	3800,	345,	376.3157895,	6.271929825,	1191.666667,	108.1907895	},
		{true	, pt::ptime(gr::date(2016, 7, 14), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 8, 12)), 1300,	19,	3900,	456,	407.6754386,	5.958333333,	1223.026316,	143			},
		{false	, pt::ptime(gr::date(2016, 8, 12), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1400,	18,	4000,	454,	0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 8, 12), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1500,	17,	4100,	343,	0,				0,				0,				0			},
		{false	, pt::ptime(gr::date(2016, 8, 12), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1600,	16,	4200,	143,	0,				0,				0,				0			},
		{true	, pt::ptime(gr::date(2016, 8, 15), pt::time_duration(1, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1700,	15,	4300,	108,	1700,			15,				4300,			108			},
		{true	, pt::ptime(gr::date(2016, 8, 15), pt::time_duration(2, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1800,	14,	4400,	102,	1800,			14,				4400,			102			},
		{true	, pt::ptime(gr::date(2016, 8, 15), pt::time_duration(3, 0, 0)),	lib::ContractExpiration(gr::date(2016, 9, 12)), 1900,	13,	4500,	100,	1900,			13,				4500,			100			},
	};

	for (size_t i = 0; i < _countof(source); ++i) {

		const Tick &tick = source[i];
		if (!tick.isCurrent) {
			continue;
		}

		if (i == 0) {

			trdk::Security::Request request = {};
			service.OnSecurityContractSwitched(tick.time, security, request);
			ASSERT_EQ(0, request.GetNumberOfTicks());
			ASSERT_EQ(request.GetTime(), pt::not_a_date_time);

		} else if (!source[i - 1].isCurrent) {

			ASSERT_LE(6, i);
			trdk::Security::Request request = {};
			service.OnSecurityContractSwitched(tick.time, security, request);
			ASSERT_EQ(0, request.GetNumberOfTicks());
			ASSERT_EQ(tick.time - pt::hours(24), request.GetTime());

			service.OnSecurityServiceEvent(
				pt::microsec_clock::local_time(),
				security,
				trdk::Security::SERVICE_EVENT_TRADING_SESSION_OPENED);
			
			for (size_t k = i - 3; k < i; ++k) {
				const Tick &historyTick = source[k];
				EXPECT_CALL(security, GetExpiration())
					.Times(1)
					.WillOnce(Return(historyTick.expiration));
				ASSERT_FALSE(
					service.OnNewTrade(
						security, 
						historyTick.time,
						trdk::ScaledPrice(historyTick.open * 100),
						3));
				ASSERT_FALSE(
					service.OnNewTrade(
						security, 
						historyTick.time,
						trdk::ScaledPrice(historyTick.low * 100),
						3));
				ASSERT_FALSE(
					service.OnNewTrade(
						security, 
						historyTick.time,
						trdk::ScaledPrice(historyTick.high * 100),
						3));
				ASSERT_FALSE(
					service.OnNewTrade(
						security, 
						historyTick.time,
						trdk::ScaledPrice(historyTick.close * 100),
						3));

			}

			service.OnSecurityServiceEvent(
				pt::microsec_clock::local_time(),
				security,
				trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED);

		}

		EXPECT_CALL(security, GetExpiration())
			.Times(1)
			.WillOnce(Return(tick.expiration));
		ASSERT_FALSE(
			service.OnNewTrade(
				security, 
				tick.time,
				trdk::ScaledPrice(tick.open * 100),
				3));
		ASSERT_FALSE(
			service.OnNewTrade(
				security, 
				tick.time,
				trdk::ScaledPrice(tick.low * 100),
				3));
		ASSERT_FALSE(
			service.OnNewTrade(
				security, 
				tick.time,
				trdk::ScaledPrice(tick.high * 100),
				3));
		ASSERT_FALSE(
			service.OnNewTrade(
				security, 
				tick.time,
				trdk::ScaledPrice(tick.close * 100),
				3));

	}

	{
		EXPECT_CALL(security, GetExpiration())
			.WillRepeatedly(
				Return(lib::ContractExpiration(gr::date(2016, 9, 12))));
		EXPECT_TRUE(
			service.OnSecurityServiceEvent(
				pt::microsec_clock::local_time(),
				security,
				trdk::Security::SERVICE_EVENT_ONLINE));
	}

	ASSERT_EQ(12, service.GetSize());

	for (size_t i = 0, barI = 0; i < _countof(source); ++i) {

		const Tick &tick = source[i];
		if (!tick.isCurrent) {
			continue;
		}

		const auto &bar = service.GetBar(barI++);
		EXPECT_EQ(tick.time, bar.time);
		EXPECT_EQ(trdk::ScaledPrice(tick.adjOpen * 100), bar.openTradePrice)
			<< i << " / " << barI;
		EXPECT_EQ(trdk::ScaledPrice(tick.adjLow * 100), bar.lowTradePrice)
			<< i << " / " << barI;
		EXPECT_EQ(trdk::ScaledPrice(tick.adjHigh * 100), bar.highTradePrice)
			<< i << " / " << barI;
		EXPECT_EQ(trdk::ScaledPrice(tick.adjClose * 100), bar.closeTradePrice)
			<< i << " / " << barI;

	}

}
