/**************************************************************************
 *   Created: 2013/11/17 13:38:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Services/BollingerBandsService.hpp"
#include "Context.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	const double source[112][4] = {
		/* source	SMA	Upper band	Lower band */
		{	1642.81	,	0.00	,	0.00,	0.00	}
	};

	struct CloseTradePiceSource {
		static trdk::ScaledPrice & GetSourcePlace(svc::BarService::Bar &bar) {
			return bar.closeTradePrice;
		}
	};

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Testing {

	template<typename Policy>
	class BollingerBandsServiceTypedTest : public testing::Test {
		

	protected:
		
		virtual void SetUp() {
			boost::format settingsString(
				"[Section]\n"
					"history = yes\n"
					"period = 10");
			const lib::IniString settings(settingsString.str());
			m_service.reset(
				new svc::BollingerBandsService(
					m_context,
					"Tag",
					lib::IniSectionRef(settings, "Section")));
		}

		virtual void TearDown() {
			m_service.reset();
		}

	protected:

		void TestOnlineResult() {

			SCOPED_TRACE(__FUNCTION__);

			for (size_t i = 0; i < _countof(source); ++i) {
			
				svc::BarService::Bar bar;
				Policy::GetSourcePlace(bar) = lib::Scale(source[i][0], 100);
			
				svc::MovingAverageService::Point ma = {source[i][1]};
			
				ASSERT_EQ(lib::IsZero(source[i][2]), lib::IsZero(source[i][3]));
				const bool hasValue = !lib::IsZero(source[i][2]);

				if (i % 2) {
					EXPECT_NE(hasValue, m_service->OnNewBar(bar))
						<< "i = " << i << ";"
						<< " source = " << Policy::GetSourcePlace(bar) << ";";
					ASSERT_EQ(hasValue, m_service->OnNewMa(ma))
						<< "i = " << i << ";"
						<< " source = " << ma.value << ";";
				} else {
					EXPECT_NE(hasValue, m_service->OnNewMa(ma))
						<< "i = " << i << ";"
						<< " source = " << ma.value << ";";
					ASSERT_EQ(hasValue, m_service->OnNewBar(bar))
						<< "i = " << i << ";"
						<< " source = " << Policy::GetSourcePlace(bar) << ";";
				}
			
				if (!hasValue) {
					continue;
				}
				const svc::BollingerBandsService::Point &point
					= m_service->GetLastPoint();
			
				EXPECT_EQ(source[i][2], lib::Descale(point.high, 100))
					<< "i = " << i << ";"
					<< " source = " << Policy::GetSourcePlace(bar) << ";"
					<< " ma = " << ma.value << ";"
					<< " high = " << point.high << ";"
					<< " low = " << point.low << ";";
			
				EXPECT_EQ(source[i][3], lib::Descale(point.low, 100))
					<< "i = " << i << ";"
					<< " source = " << Policy::GetSourcePlace(bar) << ";"
					<< " ma = " << ma.value << ";"
					<< " high = " << point.high << ";"
					<< " low = " << point.low << ";";

			}
	
		}

	protected:

		Context m_context;
		std::unique_ptr<svc::BollingerBandsService> m_service;

	};
	TYPED_TEST_CASE_P(BollingerBandsServiceTypedTest);

} }

////////////////////////////////////////////////////////////////////////////////

using namespace trdk::Testing;

TYPED_TEST_P(BollingerBandsServiceTypedTest, RealTimeWithHistory) {
	
	typedef typename TypeParam Policy;

	TestOnlineResult();

	ASSERT_NO_THROW(m_service->GetHistorySize());
	ASSERT_EQ(102, m_service->GetHistorySize());
	EXPECT_THROW(
		m_service->GetHistoryPoint(102),
		svc::MovingAverageService::ValueDoesNotExistError);
	EXPECT_THROW(
		m_service->GetHistoryPointByReversedIndex(102),
		svc::MovingAverageService::ValueDoesNotExistError);

	size_t offset = 9;
	for (size_t i = 0; i < m_service->GetHistorySize(); ++i) {
		
		ASSERT_EQ(lib::IsZero(source[i][2]), lib::IsZero(source[i][3]));
		if (lib::IsZero(source[i][2])) {
			++offset;
		}
		
		const svc::BollingerBandsService::Point &point
			= m_service->GetHistoryPoint(i);

		EXPECT_EQ(source[i + offset][2], lib::Descale(point.high, 100))
			<< "i = " << i << ";"
			<< " source = " << source[i + offset][0] << ";"
			<< " ma = " << source[i + offset][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";
			
		EXPECT_EQ(source[i + offset][3], lib::Descale(point.low, 100))
			<< "i = " << i << ";"
			<< " source = " << source[i + offset][0] << ";"
			<< " ma = " << source[i + offset][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";

	}

	offset = 0;
	for (size_t i = 0; i < m_service->GetHistorySize(); ++i) {
		
		ASSERT_EQ(
			lib::IsZero(source[111 - i - offset][2]),
			lib::IsZero(source[111 - i - offset][3]));
		if (lib::IsZero(source[111 - i - offset][2])) {
			++offset;
		}

		const svc::BollingerBandsService::Point &point
			= m_service->GetHistoryPoint(i);

		EXPECT_EQ(
				source[111 - i - offset][2],
				lib::Descale(point.high, 100))
			<< "i = " << i << ";"
			<< " source = " << source[111 - i - offset][0] << ";"
			<< " ma = " << source[111 - i - offset][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";
			
		EXPECT_EQ(
				source[111 - i - offset][3],
				lib::Descale(point.low, 100))
			<< "i = " << i << ";"
			<< " source = " << source[111 - i - offset][0] << ";"
			<< " ma = " << source[111 - i - offset][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";

	}

}

TYPED_TEST_P(BollingerBandsServiceTypedTest, RealTimeWithoutHistory) {

	TestOnlineResult();

	EXPECT_THROW(
		m_service->GetHistorySize(),
		svc::BollingerBandsService::HasNotHistory);
	EXPECT_THROW(
		m_service->GetHistoryPoint(0),
		svc::BollingerBandsService::HasNotHistory);
	EXPECT_THROW(
		m_service->GetHistoryPointByReversedIndex(0),
		svc::BollingerBandsService::HasNotHistory);

}

////////////////////////////////////////////////////////////////////////////////

REGISTER_TYPED_TEST_CASE_P(
	BollingerBandsServiceTypedTest,
	RealTimeWithHistory,
	RealTimeWithoutHistory);

namespace trdk { namespace Testing {

	typedef ::testing::Types<CloseTradePiceSource>
		BollingerBandsServiceTestPolicies;

	INSTANTIATE_TYPED_TEST_CASE_P(
		BollingerBandsService,
		BollingerBandsServiceTypedTest,
		BollingerBandsServiceTestPolicies);

} }

////////////////////////////////////////////////////////////////////////////////
