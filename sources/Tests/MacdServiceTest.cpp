/**************************************************************************
 *   Created: 2014/09/14 21:47:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

#include "Prec.hpp"
#include "Services/BollingerBandsService.hpp"
#include "Context.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	const double source[110][3] = {
		/* 	Close		MACD		MACD Signal	 */
		{	1642.81	,	0.00	,	0	},
		{	1626.13	,	0.00	,	0	},
		{	1612.52	,	0.00	,	0	},
		{	1636.36	,	0.00	,	0	},
		{	1626.73	,	0.00	,	0	},
		{	1639.04	,	0.00	,	0	},
		{	1651.81	,	0.00	,	0	},
		{	1628.93	,	0.00	,	0	},
		{	1588.19	,	0.00	,	0.00	},
		{	1592.43	,	0.00	,	0.00	},
		{	1573.09	,	0.00	,	0.00	},
		{	1588.03	,	1,609.76	,	178.86	},
		{	1603.26	,	1,608.76	,	357.61	},
		{	1613.20	,	1,609.44	,	536.44	},
		{	1606.28	,	1,608.96	,	715.21	},
		{	1614.96	,	1,609.88	,	894.09	},
		{	1614.08	,	1,610.53	,	1073.04	},
		{	1615.41	,	1,611.28	,	1252.07	},
		{	1631.89	,	1,614.45	,	1431.45	},
		{	1640.46	,	1,618.45	,	1611.28	},
		{	1652.32	,	1,623.66	,	1612.82	},
		{	1652.62	,	1,628.12	,	1614.97	},
		{	1675.02	,	1,635.33	,	1617.85	},
		{	1680.19	,	1,642.23	,	1621.55	},
		{	1682.50	,	-0.22	,	1442.65	},
		{	0.00	,	0.00	,	0.00	},
		{	1676.26	,	2.01	,	1263.92	},
		{	1680.91	,	4.11	,	1085.35	},
		{	1689.37	,	6.39	,	906.68	},
		{	1692.09	,	8.31	,	727.77	},
		{	1695.53	,	10.00	,	548.48	},
		{	1692.39	,	10.96	,	368.79	},
		{	1685.94	,	11.07	,	188.32	},
		{	1690.25	,	11.38	,	7.11	},
		{	1691.65	,	11.60	,	8.43	},
		{	1685.33	,	11.13	,	9.44	},
		{	1685.96	,	10.69	,	10.17	},
		{	1685.73	,	10.21	,	10.59	},
		{	1706.87	,	11.40	,	10.94	},
		{	1709.67	,	12.43	,	11.21	},
		{	1707.14	,	12.89	,	11.42	},
		{	1697.37	,	12.32	,	11.56	},
		{	1690.91	,	11.22	,	11.54	},
		{	1697.48	,	10.76	,	11.45	},
		{	1691.42	,	9.79	,	11.30	},
		{	1689.47	,	8.76	,	11.09	},
		{	1694.16	,	8.23	,	10.87	},
		{	1685.39	,	7.02	,	10.38	},
		{	1661.32	,	4.07	,	9.45	},
		{	1655.83	,	1.28	,	8.16	},
		{	1646.06	,	-1.70	,	6.60	},
		{	1652.35	,	-3.52	,	4.97	},
		{	1642.80	,	-5.66	,	3.14	},
		{	1656.96	,	-6.15	,	1.37	},
		{	1663.50	,	-5.94	,	-0.26	},
		{	1656.78	,	-6.24	,	-1.87	},
		{	1630.48	,	-8.50	,	-3.60	},
		{	1634.96	,	-9.82	,	-5.14	},
		{	1638.17	,	-10.49	,	-6.45	},
		{	1632.97	,	-11.31	,	-7.51	},
		{	1639.77	,	-11.28	,	-8.38	},
		{	1653.08	,	-10.06	,	-8.87	},
		{	1655.08	,	-8.84	,	-9.16	},
		{	1655.17	,	-7.77	,	-9.37	},
		{	1671.71	,	-5.52	,	-9.29	},
		{	1683.99	,	-2.72	,	-8.64	},
		{	1689.13	,	-0.08	,	-7.56	},
		{	1683.42	,	1.53	,	-6.23	},
		{	1687.99	,	3.13	,	-4.62	},
		{	1697.60	,	5.13	,	-2.80	},
		{	1704.76	,	7.20	,	-0.88	},
		{	1725.52	,	10.40	,	1.25	},
		{	1722.34	,	12.53	,	3.51	},
		{	1709.91	,	13.07	,	5.58	},
		{	1701.84	,	12.70	,	7.29	},
		{	1697.42	,	11.91	,	8.62	},
		{	1692.77	,	10.78	,	9.65	},
		{	1698.67	,	10.25	,	10.44	},
		{	1691.75	,	9.16	,	10.89	},
		{	1681.55	,	7.39	,	10.91	},
		{	1695.00	,	7.00	,	10.53	},
		{	1693.87	,	6.52	,	9.86	},
		{	1678.66	,	4.85	,	8.95	},
		{	1690.50	,	4.44	,	8.03	},
		{	1676.12	,	2.91	,	7.03	},
		{	1655.45	,	0.04	,	5.84	},
		{	1656.40	,	-2.14	,	4.46	},
		{	1692.56	,	-0.94	,	3.34	},
		{	1703.20	,	0.87	,	2.62	},
		{	1710.14	,	2.82	,	2.15	},
		{	1698.06	,	3.36	,	1.80	},
		{	1721.54	,	5.61	,	1.88	},
		{	1733.15	,	8.24	,	2.31	},
		{	1744.50	,	11.11	,	3.22	},
		{	1744.66	,	13.25	,	4.69	},
		{	1754.67	,	15.57	,	6.65	},
		{	1746.38	,	16.55	,	8.60	},
		{	1752.07	,	17.58	,	10.45	},
		{	1759.77	,	18.80	,	12.23	},
		{	1762.11	,	19.73	,	14.05	},
		{	1771.95	,	21.02	,	15.76	},
		{	1763.31	,	21.11	,	17.19	},
		{	1756.54	,	20.39	,	18.22	},
		{	1761.64	,	20.00	,	18.97	},
		{	1767.93	,	19.97	,	19.46	},
		{	1762.97	,	19.32	,	19.77	},
		{	1770.49	,	19.20	,	19.95	},
		{	1747.15	,	17.02	,	19.75	},
		{	1770.61	,	16.99	,	19.45	},
		{	1771.89	,	16.87	,	18.99	},
	};

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Testing {

	class MacdServiceTest : public testing::Test {

	protected:
		
		virtual void SetUp() {
			//...//
		}

		virtual void TearDown() {
			m_service.reset();
		}

	protected:

		Context m_context;

	};

} }

////////////////////////////////////////////////////////////////////////////////

TEST_F(MacdServiceTest, RealTimeWithHistory) {
	
	boost::format settingsString(
		"[Section]\n"
			"type = %1%\n"
			"history = yes\n"
			"periods = 12, 26, 9");
	settingsString % Policy::GetType();
	const lib::IniString settings(settingsString.str());
	Context context;

	svc::MacdService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	for (size_t i = 0; i < _countof(source); ++i) {
		svc::BarService::Bar bar;
		bar.closeTradePrice = lib::Scale(source[i][0], 100);
		ASSERT_EQ(
				!lib::IsZero(source[i][Policy::GetColumn()]),
				service.OnNewBar(bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		if (!lib::IsZero(source[i][Policy::GetColumn()])) {
			EXPECT_EQ(
					source[i][0],
					lib::Descale(service.GetLastPoint().source, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
				<< " service.GetLastPoint().value = "
					<< service.GetLastPoint().value << ";";
			EXPECT_EQ(
					source[i][Policy::GetColumn()],
					lib::Descale(service.GetLastPoint().value, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
				<< " service.GetLastPoint().value = "
					<< service.GetLastPoint().value << ";";
		}
	}

	ASSERT_NO_THROW(service.GetHistorySize());
	ASSERT_EQ(_countof(source) - 10, service.GetHistorySize());
	EXPECT_THROW(
		service.GetHistoryPoint(_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);
	EXPECT_THROW(
		service.GetHistoryPointByReversedIndex(_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);

	size_t offset = 9;
	for (size_t i = 0; i < service.GetHistorySize(); ++i) {
		auto pos = i + offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			++pos;
		}
		ASSERT_EQ(
				source[pos][Policy::GetColumn()],
				lib::Descale(service.GetHistoryPoint(i).value, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}
	offset = 0;
	for (size_t i = 0; i < service.GetHistorySize(); ++i) {
		auto pos = _countof(source) - 1 - i - offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			--pos;
		}
		ASSERT_EQ(
				source[pos][Policy::GetColumn()],
				lib::Descale(
					service.GetHistoryPointByReversedIndex(i).value,
					100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}


}

TEST_F(MacdServiceTest, RealTimeWithoutHistory) {

	typedef typename TypeParam Policy;

	boost::format settingsString(
		"[Section]\n"
			"type = %1%\n"
			"history = yes\n"
			"periods = 12, 26, 9");
	settingsString % Policy::GetType();
	const lib::IniString settings(settingsString.str());
	Context context;

	svc::MacdService service(
		context,
		"Tag",
		lib::IniSectionRef(settings, "Section"));

	for (size_t i = 0; i < _countof(source); ++i) {
		svc::BarService::Bar bar;
		bar.closeTradePrice = lib::Scale(source[i][0], 100);
		ASSERT_EQ(
				!lib::IsZero(source[i][Policy::GetColumn()]),
				service.OnNewBar(bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		if (!lib::IsZero(source[i][Policy::GetColumn()])) {
			EXPECT_EQ(
					source[i][0],
					lib::Descale(service.GetLastPoint().source, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
				<< " service.GetLastPoint().value = "
					<< service.GetLastPoint().value << ";";
			EXPECT_EQ(
					source[i][Policy::GetColumn()],
					lib::Descale(service.GetLastPoint().value, 100))
				<< "i = " << i << ";"
				<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
				<< " service.GetLastPoint().value = "
					<< service.GetLastPoint().value << ";";
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
