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
#include "BollingerBandsService.hpp"
#include "Tests/MockContext.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;

using namespace trdk::Tests;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	const double source[110][4] = {
		/* 	Close		SMA		High		Low	 */
		{	1642.81	,	0	,	0.00	,	0.00	},
		{	1626.13	,	0	,	0.00	,	0.00	},
		{	1612.52	,	0	,	0.00	,	0.00	},
		{	1636.36	,	0	,	0.00	,	0.00	},
		{	1626.73	,	0	,	0.00	,	0.00	},
		{	1639.04	,	0	,	0.00	,	0.00	},
		{	1651.81	,	0	,	0.00	,	0.00	},
		{	1628.93	,	0	,	0.00	,	0.00	},
		{	1588.19	,	0	,	0.00	,	0.00	},
		{	1592.43	,	0	,	0.00	,	0.00	},
		{	1573.09	,	0	,	0.00	,	0.00	},
		{	1588.03	,	0	,	0.00	,	0.00	},
		{	1603.26	,	0	,	0.00	,	0.00	},
		{	1613.2	,	0	,	0.00	,	0.00	},
		{	1606.28	,	0	,	0.00	,	0.00	},
		{	1614.96	,	0	,	0.00	,	0.00	},
		{	1614.08	,	0	,	0.00	,	0.00	},
		{	1615.41	,	0	,	0.00	,	0.00	},
		{	1631.89	,	0	,	0.00	,	0.00	},
		{	1640.46	,	1617.28	,	1658.23	,	1576.33	},
		{	1652.32	,	1617.76	,	1660.08	,	1575.44	},
		{	1652.62	,	1619.08	,	1663.95	,	1574.21	},
		{	1675.02	,	1622.21	,	1673.11	,	1571.31	},
		{	1680.19	,	1624.4	,	1681.01	,	1567.79	},
		{	1682.5	,	1627.19	,	1689.22	,	1565.16	},
		{	1676.26	,	1629.05	,	1694.53	,	1563.57	},
		{	1680.91	,	1630.5	,	1699.15	,	1561.85	},
		{	1689.37	,	1633.52	,	1706.79	,	1560.25	},
		{	1692.09	,	1638.72	,	1713.13	,	1564.31	},
		{	1695.53	,	1643.87	,	1719.02	,	1568.72	},
		{	1692.39	,	1649.84	,	1720.36	,	1579.32	},
		{	1685.94	,	1654.73	,	1720.87	,	1588.59	},
		{	1690.25	,	1659.08	,	1722.49	,	1595.67	},
		{	1691.65	,	1663.01	,	1724.25	,	1601.77	},
		{	1685.33	,	1666.96	,	1723.03	,	1610.89	},
		{	1685.96	,	1670.51	,	1721.74	,	1619.28	},
		{	1685.73	,	1674.09	,	1718.62	,	1629.56	},
		{	1706.87	,	1678.66	,	1716.42	,	1640.90	},
		{	1709.67	,	1682.55	,	1716.01	,	1649.09	},
		{	1707.14	,	1685.89	,	1714.90	,	1656.88	},
		{	1697.37	,	1688.14	,	1713.09	,	1663.19	},
		{	1690.91	,	1690.05	,	1708.95	,	1671.15	},
		{	1697.48	,	1691.18	,	1709.01	,	1673.35	},
		{	1691.42	,	1691.74	,	1708.84	,	1674.64	},
		{	1689.47	,	1692.09	,	1708.70	,	1675.48	},
		{	1694.16	,	1692.98	,	1707.93	,	1678.03	},
		{	1685.39	,	1693.21	,	1707.55	,	1678.87	},
		{	1661.32	,	1691.8	,	1711.76	,	1671.84	},
		{	1655.83	,	1689.99	,	1715.36	,	1664.62	},
		{	1646.06	,	1687.52	,	1719.13	,	1655.91	},
		{	1652.35	,	1685.52	,	1720.53	,	1650.51	},
		{	1642.8	,	1683.36	,	1723.01	,	1643.71	},
		{	1656.96	,	1681.69	,	1722.81	,	1640.57	},
		{	1663.5	,	1680.29	,	1721.87	,	1638.71	},
		{	1656.78	,	1678.86	,	1721.60	,	1636.12	},
		{	1630.48	,	1676.08	,	1723.55	,	1628.61	},
		{	1634.96	,	1673.55	,	1724.02	,	1623.08	},
		{	1638.17	,	1670.11	,	1720.40	,	1619.82	},
		{	1632.97	,	1666.28	,	1715.60	,	1616.96	},
		{	1639.77	,	1662.91	,	1709.75	,	1616.07	},
		{	1653.08	,	1660.69	,	1704.92	,	1616.46	},
		{	1655.08	,	1658.9	,	1700.93	,	1616.87	},
		{	1655.17	,	1656.79	,	1694.92	,	1618.66	},
		{	1671.71	,	1655.8	,	1691.22	,	1620.38	},
		{	1683.99	,	1655.53	,	1689.98	,	1621.08	},
		{	1689.13	,	1655.28	,	1688.65	,	1621.91	},
		{	1683.42	,	1655.18	,	1688.21	,	1622.15	},
		{	1687.99	,	1656.51	,	1692.45	,	1620.57	},
		{	1697.6	,	1658.6	,	1698.75	,	1618.45	},
		{	1704.76	,	1661.53	,	1705.94	,	1617.12	},
		{	0	,	0	,	0.00	,	0.00	},
		{	1725.52	,	1665.19	,	1717.35	,	1613.03	},
		{	1722.34	,	1669.17	,	1725.83	,	1612.51	},
		{	1709.91	,	1671.82	,	1730.85	,	1612.79	},
		{	1701.84	,	1673.73	,	1734.03	,	1613.43	},
		{	1697.42	,	1675.77	,	1736.38	,	1615.16	},
		{	1692.77	,	1678.88	,	1736.18	,	1621.58	},
		{	1698.67	,	1682.07	,	1736.25	,	1627.89	},
		{	1691.75	,	1684.74	,	1735.13	,	1634.35	},
		{	1681.55	,	1687.17	,	1731.69	,	1642.65	},
		{	1695	,	1689.94	,	1728.85	,	1651.03	},
		{	1693.87	,	1691.97	,	1727.03	,	1656.91	},
		{	1678.66	,	1693.15	,	1724.56	,	1661.74	},
		{	1690.5	,	1694.92	,	1721.13	,	1668.71	},
		{	1676.12	,	1695.14	,	1720.63	,	1669.65	},
		{	1655.45	,	1693.71	,	1724.24	,	1663.18	},
		{	1656.4	,	1692.08	,	1726.66	,	1657.50	},
		{	1692.56	,	1692.53	,	1726.88	,	1658.18	},
		{	1703.2	,	1693.29	,	1727.87	,	1658.71	},
		{	1710.14	,	1693.92	,	1729.24	,	1658.60	},
		{	1698.06	,	1693.59	,	1728.62	,	1658.56	},
		{	1721.54	,	1693.39	,	1727.73	,	1659.05	},
		{	1733.15	,	1693.93	,	1730.35	,	1657.51	},
		{	1744.5	,	1695.66	,	1737.79	,	1653.53	},
		{	1744.66	,	1697.8	,	1745.01	,	1650.59	},
		{	1754.67	,	1700.66	,	1753.98	,	1647.34	},
		{	1746.38	,	1703.34	,	1760.09	,	1646.59	},
		{	1752.07	,	1706.01	,	1766.53	,	1645.49	},
		{	1759.77	,	1709.41	,	1773.86	,	1644.96	},
		{	1762.11	,	1713.44	,	1780.44	,	1646.44	},
		{	1771.95	,	1717.29	,	1788.32	,	1646.26	},
		{	1763.31	,	1720.76	,	1793.64	,	1647.88	},
		{	1756.54	,	1724.65	,	1796.43	,	1652.87	},
		{	1761.64	,	1728.21	,	1799.92	,	1656.50	},
		{	1767.93	,	1732.8	,	1802.30	,	1663.30	},
		{	1762.97	,	1738.18	,	1799.01	,	1677.35	},
		{	1770.49	,	1743.88	,	1793.29	,	1694.47	},
		{	1747.15	,	1746.61	,	1790.05	,	1703.17	},
		{	1770.61	,	1749.98	,	1789.73	,	1710.23	},
		{	1771.89	,	1753.07	,	1789.41	,	1716.73	},
	};

}

////////////////////////////////////////////////////////////////////////////////

class BollingerBandsServiceTest : public testing::Test {

	protected:
		
		virtual void SetUp() {
			//...//
		}

		virtual void TearDown() {
			m_service.reset();
		}

	protected:

		void TestOnlineResult() {

			SCOPED_TRACE(__FUNCTION__);

			for (size_t i = 0; i < _countof(source); ++i) {
			
				const svc::MovingAverageService::Point ma = {
					double(lib::Scale(source[i][0], 100)),
					double(lib::Scale(source[i][1], 100))
				};
			
				ASSERT_EQ(lib::IsZero(source[i][2]), lib::IsZero(source[i][3]));
				const bool hasValue = !lib::IsZero(source[i][2]);

				EXPECT_EQ(hasValue, m_service->OnNewData(ma))
					<< "i = " << i << ";";
			
				if (!hasValue) {
					continue;
				}
				const svc::BollingerBandsService::Point &point
					= m_service->GetLastPoint();
			
				EXPECT_EQ(source[i][0], lib::Descale(point.source, 100))
					<< "i = " << i << ";"
					<< " source = " << ma.source << ";"
					<< " ma = " << ma.value << ";"
					<< " high = " << point.high << ";"
					<< " low = " << point.low << ";";

				EXPECT_EQ(source[i][2], lib::Descale(point.high, 100))
					<< "i = " << i << ";"
					<< " source = " << ma.source << ";"
					<< " ma = " << ma.value << ";"
					<< " high = " << point.high << ";"
					<< " low = " << point.low << ";";
			
				EXPECT_EQ(source[i][3], lib::Descale(point.low, 100))
					<< "i = " << i << ";"
					<< " source = " << ma.source << ";"
					<< " ma = " << ma.value << ";"
					<< " high = " << point.high << ";"
					<< " low = " << point.low << ";";

			}
	
		}

	protected:

		MockContext m_context;
		std::unique_ptr<svc::BollingerBandsService> m_service;

	};

TEST_F(BollingerBandsServiceTest, RealTimeWithHistory) {
	
	std::string settingsString(
		"[Section]\n"
			"history = yes\n");
	const lib::IniString settings(settingsString);
	m_service.reset(
		new svc::BollingerBandsService(
			m_context,
			"Tag",
			lib::IniSectionRef(settings, "Section")));

	TestOnlineResult();

	ASSERT_NO_THROW(m_service->GetHistorySize());
	ASSERT_EQ(_countof(source) - 20, m_service->GetHistorySize());
	EXPECT_THROW(
		m_service->GetHistoryPoint(_countof(source) - 20),
		svc::BollingerBandsService::ValueDoesNotExistError);
	EXPECT_THROW(
		m_service->GetHistoryPointByReversedIndex(_countof(source) - 20),
		svc::BollingerBandsService::ValueDoesNotExistError);

	size_t offset = 19;
	for (size_t i = 0; i < m_service->GetHistorySize(); ++i) {

		auto pos = i + offset;
		ASSERT_EQ(lib::IsZero(source[pos][2]), lib::IsZero(source[pos][3]));
		if (lib::IsZero(source[pos][2])) {
			++offset;
			++pos;
		}
		ASSERT_LT(pos, _countof(source));
		
		const svc::BollingerBandsService::Point &point
			= m_service->GetHistoryPoint(i);

		EXPECT_EQ(source[pos][0], lib::Descale(point.source, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";"
			<< " source = " << source[pos][0] << ";"
			<< " ma = " << source[pos][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";

		EXPECT_EQ(source[pos][2], lib::Descale(point.high, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";"
			<< " source = " << source[pos][0] << ";"
			<< " ma = " << source[pos][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";
			
		EXPECT_EQ(source[pos][3], lib::Descale(point.low, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";"
			<< " source = " << source[pos][0] << ";"
			<< " ma = " << source[pos][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";

	}

	offset = 0;
	for (size_t i = 0; i < m_service->GetHistorySize(); ++i) {
		
		auto pos = _countof(source) - 1 - i - offset;
		ASSERT_EQ(lib::IsZero(source[pos][2]), lib::IsZero(source[pos][3]));
		if (lib::IsZero(source[pos][2])) {
			++offset;
			--pos;
		}
		ASSERT_LT(0, pos);

		const svc::BollingerBandsService::Point &point
			= m_service->GetHistoryPointByReversedIndex(i);

		EXPECT_EQ(source[pos][2], lib::Descale(point.high, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";"
			<< " source = " << source[pos][0] << ";"
			<< " ma = " << source[pos][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";
			
		EXPECT_EQ(source[pos][3], lib::Descale(point.low, 100))
			<< "i = " << i << ";"
			<< " pos = " << pos << ";"
			<< " source = " << source[pos][0] << ";"
			<< " ma = " << source[pos][1] << ";"
			<< " high = " << point.high << ";"
			<< " low = " << point.low << ";";

	}

}

TEST_F(BollingerBandsServiceTest, RealTimeWithoutHistory) {

	std::string settingsString("[Section]\n");
	const lib::IniString settings(settingsString);
	m_service.reset(
		new svc::BollingerBandsService(
			m_context,
			"Tag",
			lib::IniSectionRef(settings, "Section")));

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
