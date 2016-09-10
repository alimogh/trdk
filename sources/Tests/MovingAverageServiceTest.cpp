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
#include "MockContext.hpp"
#include "MockMarketDataSource.hpp"
#include "MockDropCopy.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;

using namespace trdk::Tests;
using namespace testing;

////////////////////////////////////////////////////////////////////////////////

namespace {

	const double source[110][4] = {
		/*	Val			SMA				EMA		SMMA	*/
		{	1642.81	,	0.00	,	0.00	,	0.00	},
		{	1626.13	,	0.00	,	0.00	,	0.00	},
		{	1612.52	,	0.00	,	0.00	,	0.00	},
		{	1636.36	,	0.00	,	0.00	,	0.00	},
		{	1626.73	,	0.00	,	0.00	,	0.00	},
		{	1639.04	,	0.00	,	0.00	,	0.00	},
		{	1651.81	,	0.00	,	0.00	,	0.00	},
		{	1628.93	,	0.00	,	0.00	,	0.00	},
		{	1588.19	,	0.00	,	0.00	,	0.00	},
		{	1592.43	,	1624.50	,	1618.67	,	1624.50	},
		{	1573.09	,	1617.52	,	1610.38	,	1619.35	},
		{	1588.03	,	1613.71	,	1606.32	,	1616.22	},
		{	1603.26	,	1612.79	,	1605.76	,	1614.93	},
		{	1613.20	,	1610.47	,	1607.11	,	1614.75	},
		{	1606.28	,	1608.43	,	1606.96	,	1613.91	},
		{	1614.96	,	1606.02	,	1608.42	,	1614.01	},
		{	1614.08	,	1602.25	,	1609.45	,	1614.02	},
		{	1615.41	,	1600.89	,	1610.53	,	1614.16	},
		{	1631.89	,	1605.26	,	1614.41	,	1615.93	},
		{	1640.46	,	1610.07	,	1619.15	,	1618.38	},
		{	1652.32	,	1617.99	,	1625.18	,	1621.78	},
		{	1652.62	,	1624.45	,	1630.17	,	1624.86	},
		{	1675.02	,	1631.62	,	1638.32	,	1629.88	},
		{	1680.19	,	1638.32	,	1645.94	,	1634.91	},
		{	1682.50	,	1645.95	,	1652.58	,	1639.67	},
		{	0.00	,	0.00	,	0.00	,	0.00	},
		{	1676.26	,	1652.07 /* due accumulated double error, real value is 1652.08 */,	1656.89	,	1643.33	},
		{	1680.91	,	1658.76	,	1661.26	,	1647.09	},
		{	1689.37	,	1666.15	,	1666.37	,	1651.31	},
		{	1692.09	,	1672.17	,	1671.04	,	1655.39	},
		{	1695.53	,	1677.68	,	1675.50	,	1659.41	},
		{	1692.39	,	1681.69	,	1678.57	,	1662.70	},
		{	1685.94	,	1685.02	,	1679.91	,	1665.03	},
		{	1690.25	,	1686.54	,	1681.79	,	1667.55	},
		{	1691.65	,	1687.69	,	1683.58	,	1669.96	},
		{	1685.33	,	1687.97	,	1683.90	,	1671.50	},
		{	1685.96	,	1688.94	,	1684.27	,	1672.94	},
		{	1685.73	,	1689.42	,	1684.54	,	1674.22	},
		{	1706.87	,	1691.17	,	1688.60	,	1677.49	},
		{	1709.67	,	1692.93	,	1692.43	,	1680.70	},
		{	1707.14	,	1694.09	,	1695.10	,	1683.35	},
		{	1697.37	,	1694.59	,	1695.52	,	1684.75	},
		{	1690.91	,	1695.09	,	1694.68	,	1685.37	},
		{	1697.48	,	1695.81	,	1695.19	,	1686.58	},
		{	1691.42	,	1695.79	,	1694.50	,	1687.06	},
		{	1689.47	,	1696.20	,	1693.59	,	1687.30	},
		{	1694.16	,	1697.02	,	1693.69	,	1687.99	},
		{	1685.39	,	1696.99	,	1692.18	,	1687.73	},
		{	1661.32	,	1692.43	,	1686.57	,	1685.09	},
		{	1655.83	,	1687.05	,	1680.98	,	1682.16	},
		{	1646.06	,	1680.94	,	1674.63	,	1678.55	},
		{	1652.35	,	1676.44	,	1670.58	,	1675.93	},
		{	1642.80	,	1671.63	,	1665.53	,	1672.62	},
		{	1656.96	,	1667.58	,	1663.97	,	1671.05	},
		{	1663.50	,	1664.78	,	1663.89	,	1670.30	},
		{	1656.78	,	1661.52	,	1662.59	,	1668.95	},
		{	1630.48	,	1655.15	,	1656.76	,	1665.10	},
		{	1634.96	,	1650.10	,	1652.79	,	1662.09	},
		{	1638.17	,	1647.79	,	1650.13	,	1659.69	},
		{	1632.97	,	1645.50	,	1647.01	,	1657.02	},
		{	1639.77	,	1644.87	,	1645.70	,	1655.30	},
		{	1653.08	,	1644.95	,	1647.04	,	1655.07	},
		{	1655.08	,	1646.18	,	1648.50	,	1655.08	},
		{	1655.17	,	1646.00	,	1649.71	,	1655.08	},
		{	1671.71	,	1646.82	,	1653.71	,	1656.75	},
		{	1683.99	,	1649.54	,	1659.22	,	1659.47	},
		{	1689.13	,	1655.40	,	1664.66	,	1662.44	},
		{	1683.42	,	1660.25	,	1668.07	,	1664.54	},
		{	1687.99	,	1665.23	,	1671.69	,	1666.88	},
		{	1697.60	,	1671.69	,	1676.40	,	1669.95	},
		{	1704.76	,	1678.19	,	1681.56	,	1673.43	},
		{	1725.52	,	1685.44	,	1689.55	,	1678.64	},
		{	1722.34	,	1692.16	,	1695.51	,	1683.01	},
		{	1709.91	,	1697.64	,	1698.13	,	1685.70	},
		{	1701.84	,	1700.65	,	1698.80	,	1687.32	},
		{	1697.42	,	1701.99	,	1698.55	,	1688.33	},
		{	1692.77	,	1702.36	,	1697.50	,	1688.77	},
		{	1698.67	,	1703.88	,	1697.71	,	1689.76	},
		{	1691.75	,	1704.26	,	1696.63	,	1689.96	},
		{	1681.55	,	1702.65	,	1693.89	,	1689.12	},
		{	1695.00	,	1701.68	,	1694.09	,	1689.71	},
		{	1693.87	,	1698.51	,	1694.05	,	1690.12	},
		{	1678.66	,	1694.14	,	1691.25	,	1688.98	},
		{	1690.50	,	1692.20	,	1691.12	,	1689.13	},
		{	1676.12	,	1689.63	,	1688.39	,	1687.83	},
		{	1655.45	,	1685.43	,	1682.40	,	1684.59	},
		{	1656.40	,	1681.80	,	1677.67	,	1681.77	},
		{	1692.56	,	1681.19	,	1680.38	,	1682.85	},
		{	1703.20	,	1682.33	,	1684.53	,	1684.89	},
		{	1710.14	,	1685.19	,	1689.19	,	1687.41	},
		{	1698.06	,	1685.50	,	1690.80	,	1688.48	},
		{	1721.54	,	1688.26	,	1696.39	,	1691.78	},
		{	1733.15	,	1693.71	,	1703.07	,	1695.92	},
		{	1744.50	,	1699.11	,	1710.60	,	1700.78	},
		{	1744.66	,	1705.97	,	1716.80	,	1705.17	},
		{	1754.67	,	1715.89	,	1723.68	,	1710.12	},
		{	1746.38	,	1724.89	,	1727.81	,	1713.74	},
		{	1752.07	,	1730.84	,	1732.22	,	1717.57	},
		{	1759.77	,	1736.49	,	1737.23	,	1721.79	},
		{	1762.11	,	1741.69	,	1741.75	,	1725.83	},
		{	1771.95	,	1749.08	,	1747.24	,	1730.44	},
		{	1763.31	,	1753.26	,	1750.16	,	1733.73	},
		{	1756.54	,	1755.60	,	1751.32	,	1736.01	},
		{	1761.64	,	1757.31	,	1753.20	,	1738.57	},
		{	1767.93	,	1759.64	,	1755.88	,	1741.51	},
		{	1762.97	,	1760.47	,	1757.17	,	1743.65	},
		{	1770.49	,	1762.88	,	1759.59	,	1746.34	},
		{	1747.15	,	1762.39	,	1757.33	,	1746.42	},
		{	1770.61	,	1763.47	,	1759.74	,	1748.84	},
		{	1771.89	,	1764.45	,	1761.95	,	1751.14	},
	};

	struct SmaTrait {
		static const char * GetType() {
			return "simple";
		}
		static size_t GetColumn() {
			return 1;
		}
	};

	struct EmaTrait {
		static const char * GetType() {
			return "exponential";
		}
		static size_t GetColumn() {
			return 2;
		}
	};

	struct SmMaTrait {
		static const char * GetType() {
			return "smoothed";
		}
		static size_t GetColumn() {
			return 3;
		}
	};

}

////////////////////////////////////////////////////////////////////////////////

template<typename Policy>
class MovingAverageServiceTypedTest : public testing::Test {
	//...//
};
TYPED_TEST_CASE_P(MovingAverageServiceTypedTest);

////////////////////////////////////////////////////////////////////////////////

TYPED_TEST_P(MovingAverageServiceTypedTest, RealTimeWithHistory) {
	
	typedef typename TypeParam Policy;

	boost::format settingsString(
		"[Section]\n"
			"id = 627AF583-5B76-4001-8FF3-A110B19DE8D4\n"
			"type = %1%\n"
			"history = yes\n"
			"period = 10");
	settingsString % Policy::GetType();
	const lib::IniString settingsForBars(
		settingsString.str() + "\nsource = close price");
	const lib::IniString settingsForLastPrice(
		settingsString.str() + "\nsource = last price");
		
	MockContext context;
	MockMarketDataSource marketDataSource(0, context, std::string("0"));
	const trdk::Security security(
		context,
		lib::Symbol("TEST_SCALE2/USD::STK"),
		marketDataSource,
		true,
		trdk::Security::SupportedLevel1Types());

	MockDropCopy dropCopy;
	EXPECT_CALL(dropCopy, RegisterAbstractDataSource(_, _, _))
		.Times(2)
		.WillRepeatedly(Return(12334));
	EXPECT_CALL(dropCopy, CopyAbstractDataPoint(_, _, _))
		.Times((_countof(source) - 10) * 2);
	EXPECT_CALL(context, GetDropCopy())
		.Times((_countof(source) - 10 + 1) * 2)
		.WillRepeatedly(Return(&dropCopy));

	svc::MovingAverageService serviceForBars(
		context,
		"Tag",
		lib::IniSectionRef(settingsForBars, "Section"));
	svc::MovingAverageService serviceForLastPrice(
		context,
		"Tag",
		lib::IniSectionRef(settingsForLastPrice, "Section"));

	for (size_t i = 0; i < _countof(source); ++i) {

		const auto value = source[i][0];
			
		svc::BarService::Bar bar;
		bar.closeTradePrice = trdk::ScaledPrice(lib::Scale(value, 100));
		ASSERT_EQ(
				!lib::IsZero(source[i][Policy::GetColumn()]),
				serviceForBars.OnNewBar(security, bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		ASSERT_THROW(
			serviceForBars.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					value)),
			svc::MovingAverageService::Error);

		ASSERT_EQ(
			!lib::IsZero(source[i][Policy::GetColumn()]),
			serviceForLastPrice.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					value)));
		ASSERT_FALSE(
			serviceForLastPrice.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_BID_PRICE>(
					value)));
		ASSERT_THROW(
			serviceForLastPrice.OnNewBar(security, bar),
			svc::MovingAverageService::Error);

		if (lib::IsZero(source[i][Policy::GetColumn()])) {
			continue;
		}

		EXPECT_DOUBLE_EQ(
				source[i][0],
				serviceForBars.GetLastPoint().source)
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
			<< " service.GetLastPoint().value = "
				<< serviceForBars.GetLastPoint().value << ";";
		EXPECT_DOUBLE_EQ(
				source[i][Policy::GetColumn()],
				serviceForBars.GetLastPoint().value)
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
			<< " service.GetLastPoint().value = "
				<< serviceForBars.GetLastPoint().value << ";";

		EXPECT_DOUBLE_EQ(
			source[i][0],
			serviceForLastPrice.GetLastPoint().source);
		EXPECT_DOUBLE_EQ(
			source[i][Policy::GetColumn()],
			serviceForLastPrice.GetLastPoint().value);

	}

	ASSERT_NO_THROW(serviceForBars.GetHistorySize());
	ASSERT_EQ(_countof(source) - 10, serviceForBars.GetHistorySize());
	EXPECT_THROW(
		serviceForBars.GetHistoryPoint(_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);
	EXPECT_THROW(
		serviceForBars.GetHistoryPointByReversedIndex(_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);

	ASSERT_NO_THROW(serviceForLastPrice.GetHistorySize());
	ASSERT_EQ(_countof(source) - 10, serviceForLastPrice.GetHistorySize());
	EXPECT_THROW(
		serviceForLastPrice.GetHistoryPoint(_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);
	EXPECT_THROW(
		serviceForLastPrice.GetHistoryPointByReversedIndex(
			_countof(source) - 10),
		svc::MovingAverageService::ValueDoesNotExistError);

	size_t offset = 9;
	for (size_t i = 0; i < serviceForBars.GetHistorySize(); ++i) {
		auto pos = i + offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			++pos;
		}
		ASSERT_DOUBLE_EQ(
				source[pos][Policy::GetColumn()],
				serviceForBars.GetHistoryPoint(i).value)
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}
	offset = 0;
	for (size_t i = 0; i < serviceForBars.GetHistorySize(); ++i) {
		auto pos = _countof(source) - 1 - i - offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			--pos;
		}
		ASSERT_DOUBLE_EQ(
				source[pos][Policy::GetColumn()],
				serviceForBars.GetHistoryPointByReversedIndex(i).value)
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}

	offset = 9;
	for (size_t i = 0; i < serviceForLastPrice.GetHistorySize(); ++i) {
		auto pos = i + offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			++pos;
		}
		ASSERT_DOUBLE_EQ(
				source[pos][Policy::GetColumn()],
				serviceForLastPrice.GetHistoryPoint(i).value)
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}
	offset = 0;
	for (size_t i = 0; i < serviceForLastPrice.GetHistorySize(); ++i) {
		auto pos = _countof(source) - 1 - i - offset;
		if (trdk::Lib::IsZero(source[pos][Policy::GetColumn()])) {
			++offset;
			--pos;
		}
		ASSERT_DOUBLE_EQ(
				source[pos][Policy::GetColumn()],
				serviceForLastPrice.GetHistoryPointByReversedIndex(i).value)
			<< "i = " << i << ";"
			<< " pos = " << pos << ";";
	}

}

TYPED_TEST_P(MovingAverageServiceTypedTest, RealTimeWithoutHistory) {

	typedef typename TypeParam Policy;

	boost::format settingsString(
		"[Section]\n"
			"id = 627AF583-5B76-4001-8FF3-A110B19DE8D4\n"
			"type = %1%\n"
			"history = no\n"
			"period = 10");
	settingsString % Policy::GetType();
	const lib::IniString settingsForBars(
		settingsString.str() + "\nsource = close price");
	const lib::IniString settingsForLastPrice(
		settingsString.str() + "\nsource = last price");
		
	MockContext context;
	MockMarketDataSource marketDataSource(0, context, std::string("0"));
	const trdk::Security security(
		context,
		lib::Symbol("TEST_SCALE2/USD::STK"),
		marketDataSource,
		true,
		trdk::Security::SupportedLevel1Types());

	MockDropCopy dropCopy;
	EXPECT_CALL(dropCopy, RegisterAbstractDataSource(_, _, _))
		.Times(2)
		.WillRepeatedly(Return(12334));
	EXPECT_CALL(dropCopy, CopyAbstractDataPoint(_, _, _))
		.Times((_countof(source) - 10) * 2);
	EXPECT_CALL(context, GetDropCopy())
		.Times((_countof(source) - 10 + 1) * 2)
		.WillRepeatedly(Return(&dropCopy));

	svc::MovingAverageService serviceForBars(
		context,
		"Tag",
		lib::IniSectionRef(settingsForBars, "Section"));
	svc::MovingAverageService serviceForLastPrice(
		context,
		"Tag",
		lib::IniSectionRef(settingsForLastPrice, "Section"));

	for (size_t i = 0; i < _countof(source); ++i) {
			
		const auto &value = source[i][0];

		svc::BarService::Bar bar;
		bar.closeTradePrice = trdk::ScaledPrice(lib::Scale(value, 100));
		ASSERT_EQ(
				!lib::IsZero(source[i][Policy::GetColumn()]),
				serviceForBars.OnNewBar(security, bar))
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";";
		ASSERT_THROW(
			serviceForBars.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					value)),
			svc::MovingAverageService::Error);

		ASSERT_EQ(
			!lib::IsZero(source[i][Policy::GetColumn()]),
			serviceForLastPrice.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
					value)));
		ASSERT_FALSE(
			serviceForLastPrice.OnLevel1Tick(
				security,
				pt::not_a_date_time,
				trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_BID_PRICE>(
					value)));
		ASSERT_THROW(
			serviceForLastPrice.OnNewBar(security, bar),
			svc::MovingAverageService::Error);
			
		if (lib::IsZero(source[i][Policy::GetColumn()])) {
			continue;
		}

		EXPECT_DOUBLE_EQ(source[i][0], serviceForBars.GetLastPoint().source)
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
			<< " service.GetLastPoint().value = "
				<< serviceForBars.GetLastPoint().value << ";";
		EXPECT_DOUBLE_EQ(
				source[i][Policy::GetColumn()],
				serviceForBars.GetLastPoint().value)
			<< "i = " << i << ";"
			<< " bar.closeTradePrice = " << bar.closeTradePrice << ";"
			<< " service.GetLastPoint().value = "
				<< serviceForBars.GetLastPoint().value << ";";

		EXPECT_DOUBLE_EQ(
			source[i][0],
			serviceForLastPrice.GetLastPoint().source);
		EXPECT_DOUBLE_EQ(
			source[i][Policy::GetColumn()],
			serviceForLastPrice.GetLastPoint().value);

	}

	EXPECT_THROW(
		serviceForBars.GetHistorySize(),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		serviceForBars.GetHistoryPoint(0),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		serviceForBars.GetHistoryPointByReversedIndex(0),
		svc::MovingAverageService::HasNotHistory);

	EXPECT_THROW(
		serviceForLastPrice.GetHistorySize(),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		serviceForLastPrice.GetHistoryPoint(0),
		svc::MovingAverageService::HasNotHistory);
	EXPECT_THROW(
		serviceForLastPrice.GetHistoryPointByReversedIndex(0),
		svc::MovingAverageService::HasNotHistory);

}

////////////////////////////////////////////////////////////////////////////////

REGISTER_TYPED_TEST_CASE_P(
	MovingAverageServiceTypedTest,
	RealTimeWithHistory,
	RealTimeWithoutHistory);

////////////////////////////////////////////////////////////////////////////////

typedef ::testing::Types<SmaTrait, EmaTrait, SmMaTrait>
	MovingAverageServiceTestPolicies;

INSTANTIATE_TYPED_TEST_CASE_P(
	MovingAverageService,
	MovingAverageServiceTypedTest,
	MovingAverageServiceTestPolicies);

////////////////////////////////////////////////////////////////////////////////
