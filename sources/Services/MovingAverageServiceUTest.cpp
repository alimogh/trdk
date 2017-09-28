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
#include "Core/ContextMock.hpp"
#include "Core/DropCopyMock.hpp"
#include "Core/MarketDataSourceMock.hpp"
#include "MovingAverageService.hpp"

namespace lib = trdk::Lib;
namespace svc = trdk::Services;
namespace pt = boost::posix_time;

using namespace trdk::Tests;
using namespace testing;

////////////////////////////////////////////////////////////////////////////////

namespace {

const lib::Double source[110][4] = {
    /*            Val            SMA            EMA          SMMA */
    {1642.81000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1626.13000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1612.52000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1636.36000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1626.73000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1639.04000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1651.81000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1628.93000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1588.19000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1592.43000000000, 1624.49500000000, 1624.49500000000, 1624.49500000000},
    {1573.09000000000, 1617.52300000000, 1615.14863636364, 1619.35450000000},
    {1588.03000000000, 1613.71300000000, 1610.21797520661, 1616.22205000000},
    {1603.26000000000, 1612.78700000000, 1608.95288880541, 1614.92584500000},
    {1613.20000000000, 1610.47100000000, 1609.72509084079, 1614.75326050000},
    {1606.28000000000, 1608.42600000000, 1609.09871068792, 1613.90593445000},
    {1614.96000000000, 1606.01800000000, 1610.16439965375, 1614.01134100500},
    {1614.08000000000, 1602.24500000000, 1610.87632698943, 1614.01820690450},
    {1615.41000000000, 1600.89300000000, 1611.70063117317, 1614.15738621405},
    {1631.89000000000, 1605.26300000000, 1615.37142550532, 1615.93064759265},
    {1640.46000000000, 1610.06600000000, 1619.93298450435, 1618.38358283338},
    {1652.32000000000, 1617.98900000000, 1625.82153277629, 1621.77722455004},
    {1652.62000000000, 1624.44800000000, 1630.69398136242, 1624.86150209504},
    {1675.02000000000, 1631.62400000000, 1638.75325747834, 1629.87735188553},
    {1680.19000000000, 1638.32300000000, 1646.28721066410, 1634.90861669698},
    {1682.50000000000, 1645.94500000000, 1652.87135417972, 1639.66775502728},
    {0.00000000000, 0.00000000000, 0.00000000000, 0.00000000000},
    {1676.26000000000, 1652.07500000000, 1657.12383523795, 1643.32697952455},
    {1680.91000000000, 1658.75800000000, 1661.44859246741, 1647.08528157210},
    {1689.37000000000, 1666.15400000000, 1666.52521201879, 1651.31375341489},
    {1692.09000000000, 1672.17400000000, 1671.17335528810, 1655.39137807340},
    {1695.53000000000, 1677.68100000000, 1675.60183614481, 1659.40524026606},
    {1692.39000000000, 1681.68800000000, 1678.65422957303, 1662.70371623945},
    {1685.94000000000, 1685.02000000000, 1679.97891510520, 1665.02734461551},
    {1690.25000000000, 1686.54300000000, 1681.84638508608, 1667.54961015396},
    {1691.65000000000, 1687.68900000000, 1683.62886052497, 1669.95964913856},
    {1685.33000000000, 1687.97200000000, 1683.93815861134, 1671.49668422471},
    {1685.96000000000, 1688.94200000000, 1684.30576613655, 1672.94301580224},
    {1685.73000000000, 1689.42400000000, 1684.56471774809, 1674.22171422201},
    {1706.87000000000, 1691.17400000000, 1688.62022361207, 1677.48654279981},
    {1709.67000000000, 1692.93200000000, 1692.44745568260, 1680.70488851983},
    {1707.14000000000, 1694.09300000000, 1695.11882737668, 1683.34839966785},
    {1697.37000000000, 1694.59100000000, 1695.52813149001, 1684.75055970106},
    {1690.91000000000, 1695.08800000000, 1694.68847121910, 1685.36650373096},
    {1697.48000000000, 1695.81100000000, 1695.19602190653, 1686.57785335786},
    {1691.42000000000, 1695.78800000000, 1694.50947246898, 1687.06206802207},
    {1689.47000000000, 1696.20200000000, 1693.59320474735, 1687.30286121987},
    {1694.16000000000, 1697.02200000000, 1693.69625842965, 1687.98857509788},
    {1685.39000000000, 1696.98800000000, 1692.18602962426, 1687.72871758809},
    {1661.32000000000, 1692.43300000000, 1686.57402423803, 1685.08784582928},
    {1655.83000000000, 1687.04900000000, 1680.98420164930, 1682.16206124636},
    {1646.06000000000, 1680.94100000000, 1674.63434680397, 1678.55185512172},
    {1652.35000000000, 1676.43900000000, 1670.58264738507, 1675.93166960955},
    {1642.80000000000, 1671.62800000000, 1665.53125695142, 1672.61850264859},
    {1656.96000000000, 1667.57600000000, 1663.97284659661, 1671.05265238373},
    {1663.50000000000, 1664.78400000000, 1663.88687448814, 1670.29738714536},
    {1656.78000000000, 1661.51500000000, 1662.59471549030, 1668.94564843082},
    {1630.48000000000, 1655.14700000000, 1656.75567631024, 1665.09908358774},
    {1634.96000000000, 1650.10400000000, 1652.79282607202, 1662.08517522897},
    {1638.17000000000, 1647.78900000000, 1650.13413042256, 1659.69365770607},
    {1632.97000000000, 1645.50300000000, 1647.01337943664, 1657.02129193546},
    {1639.77000000000, 1644.87400000000, 1645.69640135725, 1655.29616274192},
    {1653.08000000000, 1644.94700000000, 1647.03887383775, 1655.07454646773},
    {1655.08000000000, 1646.17500000000, 1648.50089677634, 1655.07509182095},
    {1655.17000000000, 1645.99600000000, 1649.71346099882, 1655.08458263886},
    {1671.71000000000, 1646.81700000000, 1653.71283172631, 1656.74712437497},
    {1683.99000000000, 1649.53800000000, 1659.21777141244, 1659.47141193747},
    {1689.13000000000, 1655.40300000000, 1664.65635842836, 1662.43727074373},
    {1683.42000000000, 1660.24900000000, 1668.06792962320, 1664.53554366935},
    {1687.99000000000, 1665.23100000000, 1671.69012423716, 1666.88098930242},
    {1697.60000000000, 1671.69400000000, 1676.40101073950, 1669.95289037218},
    {1704.76000000000, 1678.19300000000, 1681.55719060504, 1673.43360133496},
    {1725.52000000000, 1685.43700000000, 1689.55042867685, 1678.64224120146},
    {1722.34000000000, 1692.16300000000, 1695.51216891743, 1683.01201708132},
    {1709.91000000000, 1697.63700000000, 1698.12995638698, 1685.70181537318},
    {1701.84000000000, 1700.65000000000, 1698.80450977117, 1687.31563383587},
    {1697.42000000000, 1701.99300000000, 1698.55278072187, 1688.32607045228},
    {1692.77000000000, 1702.35700000000, 1697.50136604516, 1688.77046340705},
    {1698.67000000000, 1703.88200000000, 1697.71384494604, 1689.76041706635},
    {1691.75000000000, 1704.25800000000, 1696.62950950131, 1689.95937535971},
    {1681.55000000000, 1702.65300000000, 1693.88778050107, 1689.11843782374},
    {1695.00000000000, 1701.67700000000, 1694.09000222815, 1689.70659404137},
    {1693.87000000000, 1698.51200000000, 1694.05000182303, 1690.12293463723},
    {1678.66000000000, 1694.14400000000, 1691.25181967339, 1688.97664117351},
    {1690.50000000000, 1692.20300000000, 1691.11512518732, 1689.12897705616},
    {1676.12000000000, 1689.63100000000, 1688.38873878962, 1687.82807935054},
    {1655.45000000000, 1685.43400000000, 1682.39987719151, 1684.59027141549},
    {1656.40000000000, 1681.79700000000, 1677.67262679305, 1681.77124427394},
    {1692.56000000000, 1681.18600000000, 1680.37942192159, 1682.85011984654},
    {1703.20000000000, 1682.33100000000, 1684.52861793585, 1684.88510786189},
    {1710.14000000000, 1685.19000000000, 1689.18523285660, 1687.41059707570},
    {1698.06000000000, 1685.49600000000, 1690.79882688267, 1688.47553736813},
    {1721.54000000000, 1688.26300000000, 1696.38813108582, 1691.78198363132},
    {1733.15000000000, 1693.71200000000, 1703.07210725204, 1695.91878526819},
    {1744.50000000000, 1699.11200000000, 1710.60445138803, 1700.77690674137},
    {1744.66000000000, 1705.96600000000, 1716.79636931748, 1705.16521606723},
    {1754.67000000000, 1715.88800000000, 1723.68248398703, 1710.11569446051},
    {1746.38000000000, 1724.88600000000, 1727.80930508030, 1713.74212501446},
    {1752.07000000000, 1730.83700000000, 1732.22034052024, 1717.57491251301},
    {1759.77000000000, 1736.49400000000, 1737.22936951656, 1721.79442126171},
    {1762.11000000000, 1741.69100000000, 1741.75312051355, 1725.82597913554},
    {1771.95000000000, 1749.08000000000, 1747.24346223836, 1730.43838122199},
    {1763.31000000000, 1753.25700000000, 1750.16465092229, 1733.72554309979},
    {1756.54000000000, 1755.59600000000, 1751.32380530006, 1736.00698878981},
    {1761.64000000000, 1757.31000000000, 1753.19947706368, 1738.57028991083},
    {1767.93000000000, 1759.63700000000, 1755.87775396120, 1741.50626091974},
    {1762.97000000000, 1760.46700000000, 1757.16725324098, 1743.65263482777},
    {1770.49000000000, 1762.87800000000, 1759.58957083353, 1746.33637134499},
    {1747.15000000000, 1762.38600000000, 1757.32783068198, 1746.41773421049},
    {1770.61000000000, 1763.47000000000, 1759.74277055798, 1748.83696078944},
    {1771.89000000000, 1764.44800000000, 1761.95135772926, 1751.14226471050},
};

struct SmaTrait {
  static const char *GetType() { return "simple"; }
  static size_t GetColumn() { return 1; }
};

struct EmaTrait {
  static const char *GetType() { return "exponential"; }
  static size_t GetColumn() { return 2; }
};

struct SmMaTrait {
  static const char *GetType() { return "smoothed"; }
  static size_t GetColumn() { return 3; }
};
}

////////////////////////////////////////////////////////////////////////////////

template <typename Policy>
class MovingAverageServiceTyped : public testing::Test {};
TYPED_TEST_CASE_P(MovingAverageServiceTyped);

////////////////////////////////////////////////////////////////////////////////

TYPED_TEST_P(MovingAverageServiceTyped, RealTimeWithHistory) {
  typedef typename TypeParam Policy;

  boost::format settingsString(
      "[Section]\n"
      "type = %1%\n"
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "history = yes\n"
      "period = 10\n"
      "log = none");
  settingsString % Policy::GetType();
  const lib::IniString settingsForBars(settingsString.str() +
                                       "\nsource = close price");
  const lib::IniString settingsForLastPrice(settingsString.str() +
                                            "\nsource = last price");

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());

  svc::MovingAverageService serviceForBars(
      context, "Test", lib::IniSectionRef(settingsForBars, "Section"));
  svc::MovingAverageService serviceForLastPrice(
      context, "Test", lib::IniSectionRef(settingsForLastPrice, "Section"));

  for (size_t i = 0; i < _countof(source); ++i) {
    const auto value = source[i][0];

    svc::BarService::Bar bar;
    bar.closeTradePrice = value;
    ASSERT_EQ(source[i][Policy::GetColumn()] != 0,
              serviceForBars.OnNewBar(security, bar))
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";";
    ASSERT_THROW(
        serviceForBars.OnLevel1Tick(
            security, pt::not_a_date_time,
            trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(value)),
        svc::MovingAverageService::Error);

    ASSERT_EQ(source[i][Policy::GetColumn()] != 0,
              serviceForLastPrice.OnLevel1Tick(
                  security, pt::not_a_date_time,
                  trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
                      value)));
    ASSERT_FALSE(serviceForLastPrice.OnLevel1Tick(
        security, pt::not_a_date_time,
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_BID_PRICE>(value)));
    ASSERT_THROW(serviceForLastPrice.OnNewBar(security, bar),
                 svc::MovingAverageService::Error);

    if (!source[i][Policy::GetColumn()]) {
      continue;
    }

    EXPECT_EQ(source[i][0], serviceForBars.GetLastPoint().source)
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";"
        << " service.GetLastPoint().value = "
        << serviceForBars.GetLastPoint().value << ";";
    EXPECT_NEAR(source[i][Policy::GetColumn()],
                serviceForBars.GetLastPoint().value, 0.00000000001)
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";"
        << " service.GetLastPoint().value = "
        << serviceForBars.GetLastPoint().value << ";";

    EXPECT_EQ(source[i][0], serviceForLastPrice.GetLastPoint().source);
    EXPECT_NEAR(source[i][Policy::GetColumn()],
                serviceForLastPrice.GetLastPoint().value, 0.00000000001);
  }

  ASSERT_NO_THROW(serviceForBars.GetHistorySize());
  ASSERT_EQ(_countof(source) - 10, serviceForBars.GetHistorySize());
  EXPECT_THROW(serviceForBars.GetHistoryPoint(_countof(source) - 10),
               svc::MovingAverageService::ValueDoesNotExistError);
  EXPECT_THROW(
      serviceForBars.GetHistoryPointByReversedIndex(_countof(source) - 10),
      svc::MovingAverageService::ValueDoesNotExistError);

  ASSERT_NO_THROW(serviceForLastPrice.GetHistorySize());
  ASSERT_EQ(_countof(source) - 10, serviceForLastPrice.GetHistorySize());
  EXPECT_THROW(serviceForLastPrice.GetHistoryPoint(_countof(source) - 10),
               svc::MovingAverageService::ValueDoesNotExistError);
  EXPECT_THROW(
      serviceForLastPrice.GetHistoryPointByReversedIndex(_countof(source) - 10),
      svc::MovingAverageService::ValueDoesNotExistError);

  size_t offset = 9;
  for (size_t i = 0; i < serviceForBars.GetHistorySize(); ++i) {
    auto pos = i + offset;
    if (!source[pos][Policy::GetColumn()]) {
      ++offset;
      ++pos;
    }
    EXPECT_NEAR(source[pos][Policy::GetColumn()],
                serviceForBars.GetHistoryPoint(i).value, 0.00000000001)
        << "i = " << i << ";"
        << " pos = " << pos << ";";
  }
  offset = 0;
  for (size_t i = 0; i < serviceForBars.GetHistorySize(); ++i) {
    auto pos = _countof(source) - 1 - i - offset;
    if (!source[pos][Policy::GetColumn()]) {
      ++offset;
      --pos;
    }
    EXPECT_NEAR(source[pos][Policy::GetColumn()],
                serviceForBars.GetHistoryPointByReversedIndex(i).value,
                0.00000000001)
        << "i = " << i << ";"
        << " pos = " << pos << ";";
  }

  offset = 9;
  for (size_t i = 0; i < serviceForLastPrice.GetHistorySize(); ++i) {
    auto pos = i + offset;
    if (!source[pos][Policy::GetColumn()]) {
      ++offset;
      ++pos;
    }
    EXPECT_NEAR(source[pos][Policy::GetColumn()],
                serviceForLastPrice.GetHistoryPoint(i).value, 0.00000000001)
        << "i = " << i << ";"
        << " pos = " << pos << ";";
  }
  offset = 0;
  for (size_t i = 0; i < serviceForLastPrice.GetHistorySize(); ++i) {
    auto pos = _countof(source) - 1 - i - offset;
    if (!source[pos][Policy::GetColumn()]) {
      ++offset;
      --pos;
    }
    EXPECT_NEAR(source[pos][Policy::GetColumn()],
                serviceForLastPrice.GetHistoryPointByReversedIndex(i).value,
                0.00000000001)
        << "i = " << i << ";"
        << " pos = " << pos << ";";
  }
}

TYPED_TEST_P(MovingAverageServiceTyped, RealTimeWithoutHistory) {
  typedef typename TypeParam Policy;

  boost::format settingsString(
      "[Section]\n"
      "type = %1%\n"
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "history = no\n"
      "period = 10\n"
      "log = none");
  settingsString % Policy::GetType();
  const lib::IniString settingsForBars(settingsString.str() +
                                       "\nsource = close price");
  const lib::IniString settingsForLastPrice(settingsString.str() +
                                            "\nsource = last price");

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());

  svc::MovingAverageService serviceForBars(
      context, "Test", lib::IniSectionRef(settingsForBars, "Section"));
  svc::MovingAverageService serviceForLastPrice(
      context, "Test", lib::IniSectionRef(settingsForLastPrice, "Section"));

  pt::ptime time = pt::microsec_clock::local_time();
  for (size_t i = 0; i < _countof(source); ++i) {
    const auto &value = source[i][0];
    time += pt::seconds(123);

    svc::BarService::Bar bar;
    bar.endTime = time;
    bar.closeTradePrice = value;
    ASSERT_EQ(source[i][Policy::GetColumn()] != 0,
              serviceForBars.OnNewBar(security, bar))
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";";
    ASSERT_THROW(
        serviceForBars.OnLevel1Tick(
            security, time,
            trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(value)),
        svc::MovingAverageService::Error);
    ASSERT_EQ(source[i][Policy::GetColumn()] != 0,
              serviceForLastPrice.OnLevel1Tick(
                  security, time,
                  trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
                      value)));
    ASSERT_FALSE(serviceForLastPrice.OnLevel1Tick(
        security, time,
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_BID_PRICE>(value)));
    ASSERT_THROW(serviceForLastPrice.OnNewBar(security, bar),
                 svc::MovingAverageService::Error);

    if (!source[i][Policy::GetColumn()]) {
      continue;
    }

    EXPECT_EQ(time, serviceForBars.GetLastPoint().time);
    EXPECT_DOUBLE_EQ(source[i][0], serviceForBars.GetLastPoint().source)
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";"
        << " service.GetLastPoint().value = "
        << serviceForBars.GetLastPoint().value << ";";
    EXPECT_NEAR(source[i][Policy::GetColumn()],
                serviceForBars.GetLastPoint().value, 0.00000000001)
        << "i = " << i << ";"
        << " bar.closeTradePrice = " << bar.closeTradePrice << ";"
        << " service.GetLastPoint().value = "
        << serviceForBars.GetLastPoint().value << ";";

    EXPECT_DOUBLE_EQ(source[i][0], serviceForLastPrice.GetLastPoint().source);
    EXPECT_NEAR(source[i][Policy::GetColumn()],
                serviceForLastPrice.GetLastPoint().value, 0.00000000001);
  }

  EXPECT_THROW(serviceForBars.GetHistorySize(),
               svc::MovingAverageService::HasNotHistory);
  EXPECT_THROW(serviceForBars.GetHistoryPoint(0),
               svc::MovingAverageService::HasNotHistory);
  EXPECT_THROW(serviceForBars.GetHistoryPointByReversedIndex(0),
               svc::MovingAverageService::HasNotHistory);

  EXPECT_THROW(serviceForLastPrice.GetHistorySize(),
               svc::MovingAverageService::HasNotHistory);
  EXPECT_THROW(serviceForLastPrice.GetHistoryPoint(0),
               svc::MovingAverageService::HasNotHistory);
  EXPECT_THROW(serviceForLastPrice.GetHistoryPointByReversedIndex(0),
               svc::MovingAverageService::HasNotHistory);
}

////////////////////////////////////////////////////////////////////////////////

REGISTER_TYPED_TEST_CASE_P(MovingAverageServiceTyped,
                           RealTimeWithHistory,
                           RealTimeWithoutHistory);

////////////////////////////////////////////////////////////////////////////////

typedef ::testing::Types<SmaTrait, EmaTrait, SmMaTrait>
    MovingAverageServiceTestPolicies;

INSTANTIATE_TYPED_TEST_CASE_P(MovingAverageService,
                              MovingAverageServiceTyped,
                              MovingAverageServiceTestPolicies);

////////////////////////////////////////////////////////////////////////////////

TEST(Services_MovingAverageServiceTyped, DropCopy) {
  const lib::IniString settings(
      "[Section]\n"
      "id = 00000000-0000-0000-0000-000000000001\n"
      "type = simple\n"
      "source = last price\n"
      "history = no\n"
      "period = 2\n"
      "log = none");

  Mocks::DropCopy dropCopy;

  pt::ptime time = pt::microsec_clock::local_time();

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());

  svc::MovingAverageService service(context, "Test",
                                    lib::IniSectionRef(settings, "Section"));

  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    EXPECT_THROW(service.DropLastPointCopy(11),
                 svc::MovingAverageService::ValueDoesNotExistError);
  }

  time += pt::seconds(12);
  EXPECT_FALSE(service.OnLevel1Tick(
      security, time,
      trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(110)));
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    EXPECT_THROW(service.DropLastPointCopy(11),
                 svc::MovingAverageService::ValueDoesNotExistError);
  }

  time += pt::seconds(12);
  ASSERT_TRUE(service.OnLevel1Tick(
      security, time,
      trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(130)));
  {
    const auto value = service.GetLastPoint();
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyAbstractData(13, 0, value.time, value.value))
        .Times(1);
    service.DropLastPointCopy(13);
  }

  time += pt::seconds(12);
  ASSERT_TRUE(service.OnLevel1Tick(
      security, time,
      trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(140)));
  {
    const auto value = service.GetLastPoint();
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyAbstractData(14, 1, value.time, value.value))
        .Times(1);
    service.DropLastPointCopy(14);
  }

  time += pt::seconds(12);
  ASSERT_TRUE(service.OnLevel1Tick(
      security, time,
      trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(150)));
  {
    const auto value = service.GetLastPoint();
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyAbstractData(15, 2, value.time, value.value))
        .Times(1);
    service.DropLastPointCopy(15);
  }
}

////////////////////////////////////////////////////////////////////////////////
