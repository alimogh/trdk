/**************************************************************************
 *   Created: 2013/11/12 06:55:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Services/MovingAverageService.hpp"
#include "Context.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace {

	const double movingAverageSimpleSource[100][2] = {
			{	2394	,	0	},
			{	2697	,	0	},
			{	8934	,	0	},
			{	1254	,	3819.75	},
			{	5381	,	4566.5	},
			{	8976	,	6136.25	},
			{	2667	,	4569.5	},
			{	1026	,	4512.5	},
			{	9843	,	5628	},
			{	4722	,	4564.5	},
			{	8866	,	6114.25	},
			{	834	,	6066.25	},
			{	1857	,	4069.75	},
			{	2864	,	3605.25	},
			{	4778	,	2583.25	},
			{	753	,	2563	},
			{	4622	,	3254.25	},
			{	949	,	2775.5	},
			{	805	,	1782.25	},
			{	1861	,	2059.25	},
			{	1976	,	1397.75	},
			{	2095	,	1684.25	},
			{	5112	,	2761	},
			{	4284	,	3366.75	},
			{	9547	,	5259.5	},
			{	8371	,	6828.5	},
			{	9484	,	7921.5	},
			{	3219	,	7655.25	},
			{	165	,	5309.75	},
			{	5896	,	4691	},
			{	6193	,	3868.25	},
			{	7596	,	4962.5	},
			{	3325	,	5752.5	},
			{	1117	,	4557.75	},
			{	4277	,	4078.75	},
			{	5166	,	3471.25	},
			{	9152	,	4928	},
			{	4200	,	5698.75	},
			{	6839	,	6339.25	},
			{	2906	,	5774.25	},
			{	5913	,	4964.5	},
			{	5654	,	5328	},
			{	5677	,	5037.5	},
			{	8663	,	6476.75	},
			{	2676	,	5667.5	},
			{	3623	,	5159.75	},
			{	4363	,	4831.25	},
			{	6051	,	4178.25	},
			{	1872	,	3977.25	},
			{	9161	,	5361.75	},
			{	9377	,	6615.25	},
			{	7609	,	7004.75	},
			{	1195	,	6835.5	},
			{	4042	,	5555.75	},
			{	997	,	3460.75	},
			{	756	,	1747.5	},
			{	9716	,	3877.75	},
			{	218	,	2921.75	},
			{	369	,	2764.75	},
			{	8612	,	4728.75	},
			{	1978	,	2794.25	},
			{	1363	,	3080.5	},
			{	9596	,	5387.25	},
			{	7845	,	5195.5	},
			{	2893	,	5424.25	},
			{	9347	,	7420.25	},
			{	5153	,	6309.5	},
			{	1150	,	4635.75	},
			{	8002	,	5913	},
			{	690	,	3748.75	},
			{	3113	,	3238.75	},
			{	9447	,	5313	},
			{	5438	,	4672	},
			{	1703	,	4925.25	},
			{	1084	,	4418	},
			{	7170	,	3848.75	},
			{	5464	,	3855.25	},
			{	3502	,	4305	},
			{	6965	,	5775.25	},
			{	6974	,	5726.25	},
			{	4467	,	5477	},
			{	9769	,	7043.75	},
			{	2310	,	5880	},
			{	8498	,	6261	},
			{	3662	,	6059.75	},
			{	7536	,	5501.5	},
			{	3986	,	5920.5	},
			{	1287	,	4117.75	},
			{	2994	,	3950.75	},
			{	2475	,	2685.5	},
			{	1903	,	2164.75	},
			{	9700	,	4268	},
			{	6687	,	5191.25	},
			{	3700	,	5497.5	},
			{	7493	,	6895	},
			{	6324	,	6051	},
			{	9636	,	6788.25	},
			{	2388	,	6460.25	},
			{	9668	,	7004	},
			{	8653	,	7586.25	}};

	struct Simple {

		typedef double Source[100][2];

		static const char * GetSettingsWithHistory() {
			return
				"[MovingAverageSection]\n"
					"type = simple\n"
					"history = yes\n"
					"period = 4";
		}

		static const char * GetSettingsWithoutHistory() {
			return
				"[MovingAverageSection]\n"
					"type = simple\n"
					"period = 4";
		}

		static const Source & GetSource() {
			return movingAverageSimpleSource;
		}

	};

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Testing {

	template<typename Policy>
	class MovingAverageServiceTypedTest : public testing::Test {
		//...//
	};
	TYPED_TEST_CASE_P(MovingAverageServiceTypedTest);

} }

////////////////////////////////////////////////////////////////////////////////

using namespace trdk::Testing;

TYPED_TEST_P(MovingAverageServiceTypedTest, RealTimeWithHistory) {
	
	typedef typename TypeParam Policy;

	const lib::IniString settings(Policy::GetSettingsWithHistory());
	Context context;

	svc::MovingAverageService service(
		context,
		"MovingAverageTag",
		lib::IniFileSectionRef(settings, "MovingAverageSection"));

	for (size_t i = 0; i < _countof(Policy::GetSource()); ++i) {
		svc::BarService::Bar bar;
		bar.closeTradePrice
			= lib::Scale(Policy::GetSource()[i][0], 100);
		ASSERT_EQ(i >= 3, service.OnNewBar(bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		if (!lib::IsZero(Policy::GetSource()[i][1])) {
			ASSERT_EQ(
					Policy::GetSource()[i][1],
					lib::Descale(service.GetLastPoint().value, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		}
	}

	ASSERT_NO_THROW(service.GetHistorySize());
	ASSERT_EQ(97, service.GetHistorySize());
	EXPECT_THROW(
		service.GetHistoryPoint(97),
		svc::MovingAverageService::ValueDoesNotExistError);
	EXPECT_THROW(
		service.GetHistoryPointByReversedIndex(97),
		svc::MovingAverageService::ValueDoesNotExistError);

	for (size_t i = 0; i < service.GetHistorySize(); ++i) {
		ASSERT_EQ(
				lib::Scale(Policy::GetSource()[i + 3][1], 100),
				service.GetHistoryPoint(i).value)
			<< "i = " << i << ";";
	}
	for (size_t i = 0; i < service.GetHistorySize(); ++i) {
		ASSERT_EQ(
				lib::Scale(Policy::GetSource()[99 - i][1], 100),
				service.GetHistoryPointByReversedIndex(i).value)
			<< "i = " << i << ";";
	}

}

TYPED_TEST_P(MovingAverageServiceTypedTest, RealTimeWithoutHistory) {

	typedef typename TypeParam Policy;

	const lib::IniString settings(Policy::GetSettingsWithoutHistory());
	Context context;

	svc::MovingAverageService service(
		context,
		"MovingAverageTag",
		lib::IniFileSectionRef(settings, "MovingAverageSection"));

	for (size_t i = 0; i < _countof(Policy::GetSource()); ++i) {
		svc::BarService::Bar bar;
		bar.closeTradePrice
			= lib::Scale(Policy::GetSource()[i][0], 100);
		ASSERT_EQ(i >= 3, service.OnNewBar(bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		if (!lib::IsZero(Policy::GetSource()[i][1])) {
			ASSERT_EQ(
					Policy::GetSource()[i][1],
					lib::Descale(service.GetLastPoint().value, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		}
	}

	EXPECT_THROW(
		service.GetHistorySize(),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		service.GetHistoryPoint(0),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		service.GetHistoryPointByReversedIndex(0),
		svc::MovingAverageService::HasNotHistory);

}

////////////////////////////////////////////////////////////////////////////////


REGISTER_TYPED_TEST_CASE_P(
	MovingAverageServiceTypedTest,
	RealTimeWithHistory,
	RealTimeWithoutHistory);

namespace trdk { namespace Testing {

	typedef ::testing::Types<Simple>
		MovingAverageServiceTestPolicies;

	INSTANTIATE_TYPED_TEST_CASE_P(
		MovingAverageService,
		MovingAverageServiceTypedTest,
		MovingAverageServiceTestPolicies);

} }

////////////////////////////////////////////////////////////////////////////////
