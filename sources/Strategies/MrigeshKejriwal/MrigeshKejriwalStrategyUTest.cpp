/*******************************************************************************
 *   Created: 2017/07/22 21:44:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Core/ContextMock.hpp"
#include "Core/SecurityMock.hpp"
#include "Core/ServiceMock.hpp"
#include "Core/TradingSystemMock.hpp"
#include "Services/MovingAverageServiceMock.hpp"
#include "MrigeshKejriwalStrategy.hpp"

using namespace testing;
using namespace trdk::Tests;

namespace mk = trdk::Strategies::MrigeshKejriwal;
namespace svc = trdk::Services;
namespace tms = trdk::Lib::TimeMeasurement;
namespace lib = trdk::Lib;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

namespace {
class TrendMock : public mk::Trend {
 public:
  virtual ~TrendMock() {}

 public:
  MOCK_METHOD2(Update,
               bool(const trdk::Price &,
                    const svc::MovingAverageService::Point &));
  MOCK_CONST_METHOD0(IsRising, const boost::tribool &());
};
}

TEST(MrigeshKejriwal_Strategy, Setup) {
  lib::IniString settings(
      "[Section]\n"
      "id = {00000000-0000-0000-0000-000000000000}\n"
      "title = Test\n"
      "is_enabled = true\n"
      "trading_mode = paper\n"
      "qty=99\n"
      "history_hours=129\n"
      "cost_of_funds=0.12\n");
  const auto &currentTime = pt::microsec_clock::local_time();

  Mocks::Context context;
  EXPECT_CALL(context, GetDropCopy()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(context, GetCurrentTime()).WillRepeatedly(Return(currentTime));

  mk::Strategy strategy(context, "test",
                        lib::IniSectionRef(settings, "Section"));

  const lib::Symbol tradingSymbol("XXX*/USD::FUT");
  Mocks::Security tradingSecurity;
  const lib::ContractExpiration tradingSecurityContractExpiration(
      gr::date(2017, 6, 4));
  EXPECT_CALL(tradingSecurity, GetExpiration())
      .WillRepeatedly(ReturnRef(tradingSecurityContractExpiration));
  tradingSecurity.SetSymbolToMock(tradingSymbol);
  {
    tradingSecurity.SetRequest(trdk::Security::Request());
    strategy.RegisterSource(tradingSecurity);
    EXPECT_TRUE(tradingSecurity.GetRequest().GetTime().is_not_a_date_time());
    EXPECT_EQ(0, tradingSecurity.GetRequest().GetNumberOfTicks());
  }
  const lib::Symbol tradingSymbol2("XXX2*/USD::FUT");
  Mocks::Security tradingSecurity2;
  tradingSecurity2.SetSymbolToMock(tradingSymbol2);
  EXPECT_THROW(strategy.RegisterSource(tradingSecurity2), lib::Exception);
  EXPECT_TRUE(tradingSecurity2.GetRequest().GetTime().is_not_a_date_time());
  EXPECT_EQ(0, tradingSecurity2.GetRequest().GetNumberOfTicks());

  const lib::Symbol spotSymbol("XXX/USD::INDEX");
  Mocks::Security spotSecurity;
  spotSecurity.SetSymbolToMock(spotSymbol);
  {
    strategy.RegisterSource(spotSecurity);
    EXPECT_TRUE(tradingSecurity.GetRequest().GetTime().is_not_a_date_time());
    EXPECT_EQ(0, spotSecurity.GetRequest().GetNumberOfTicks());
  }
  const lib::Symbol spotSymbol2("XXX2/USD::INDEX");
  Mocks::Security spotSecurity2;
  spotSecurity2.SetSymbolToMock(spotSymbol2);
  EXPECT_THROW(strategy.RegisterSource(spotSecurity2), lib::Exception);
  EXPECT_TRUE(spotSecurity2.GetRequest().GetTime().is_not_a_date_time());
  EXPECT_EQ(0, spotSecurity2.GetRequest().GetNumberOfTicks());

  Mocks::Service unknownService;
  EXPECT_NO_THROW(strategy.OnServiceStart(unknownService));
  Mocks::MovingAverageService maService;
  EXPECT_NO_THROW(strategy.OnServiceStart(maService));
  EXPECT_THROW(strategy.OnServiceStart(maService), trdk::Lib::Exception);
  EXPECT_NO_THROW(strategy.OnServiceStart(unknownService));
}

TEST(MrigeshKejriwal_Strategy, DISABLED_Position) {
  const lib::IniString settings(
      "[Section]\n"
      "id = {00000000-0000-0000-0000-000000000000}\n"
      "title = Test\n"
      "is_enabled = true\n"
      "trading_mode = paper\n"
      "qty=99\n"
      "cost_of_funds=0.12\n");
  const auto &currentTime = pt::microsec_clock::local_time();

  Mocks::Context context;
  EXPECT_CALL(context, GetDropCopy()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(context, GetCurrentTime()).WillRepeatedly(Return(currentTime));

  auto trend = boost::make_shared<TrendMock>();

  mk::Strategy strategy(context, "test",
                        lib::IniSectionRef(settings, "Section"), trend);

  const lib::Symbol tradingSymbol("XXX*/USD::FUT");
  Mocks::Security tradingSecurity;
  EXPECT_CALL(tradingSecurity, IsOnline()).WillRepeatedly(Return(true));
  EXPECT_CALL(tradingSecurity, IsTradingSessionOpened())
      .WillRepeatedly(Return(true));
  const lib::ContractExpiration tradingSecurityContractExpiration(
      gr::date(2017, 6, 4));
  EXPECT_CALL(tradingSecurity, GetExpiration())
      .WillRepeatedly(ReturnRef(tradingSecurityContractExpiration));
  tradingSecurity.SetSymbolToMock(tradingSymbol);
  strategy.RegisterSource(tradingSecurity);

  const lib::Symbol spotSymbol("XXX/USD::INDEX");
  Mocks::Security spotSecurity;
  EXPECT_CALL(spotSecurity, IsOnline()).Times(0);
  EXPECT_CALL(spotSecurity, IsTradingSessionOpened()).Times(0);
  EXPECT_CALL(spotSecurity, GetExpiration()).Times(0);
  spotSecurity.SetSymbolToMock(spotSymbol);
  strategy.RegisterSource(spotSecurity);

  Mocks::MovingAverageService ma;
  strategy.OnServiceStart(ma);

  {
    EXPECT_CALL(ma, GetLastPoint()).Times(0);
    EXPECT_CALL(ma, IsEmpty()).Times(0);
    EXPECT_CALL(*trend, Update(_, _)).Times(0);
    EXPECT_CALL(*trend, IsRising()).Times(0);
    strategy.RaiseServiceDataUpdateEvent(ma, tms::Milestones());
    strategy.RaiseServiceDataUpdateEvent(ma, tms::Milestones());
  }

  {
    EXPECT_CALL(ma, GetLastPoint()).Times(0);
    EXPECT_CALL(ma, IsEmpty()).Times(0);
    EXPECT_CALL(*trend, Update(_, _)).Times(0);
    EXPECT_CALL(*trend, IsRising()).Times(0);
    strategy.RaiseLevel1TickEvent(
        tradingSecurity, pt::ptime(),
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(0),
        tms::Milestones());
  }

  {
    EXPECT_CALL(ma, GetLastPoint()).Times(0);
    EXPECT_CALL(ma, IsEmpty()).WillRepeatedly(Return(true));
    EXPECT_CALL(*trend, Update(_, _)).Times(0);
    EXPECT_CALL(*trend, IsRising()).Times(0);
    strategy.RaiseLevel1TickEvent(
        spotSecurity, pt::ptime(),
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(0),
        tms::Milestones());
  }

  const svc::MovingAverageService::Point point = {
      currentTime + pt::seconds(123), 1.11, 2.22};
  EXPECT_CALL(ma, GetLastPoint()).WillRepeatedly(ReturnRef(point));
  EXPECT_CALL(ma, IsEmpty()).WillRepeatedly(Return(false));

  {
    EXPECT_CALL(*trend, Update(trdk::Price(98.65), point))
        .WillOnce(Return(false));
    EXPECT_CALL(*trend, IsRising()).Times(0);
    strategy.RaiseLevel1TickEvent(
        spotSecurity, pt::ptime(),
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(9865),
        tms::Milestones());
  }

  Mocks::TradingSystem tradingSystem;
  EXPECT_CALL(context, GetTradingSystem(0, trdk::TRADING_MODE_PAPER))
      .Times(2)
      .WillRepeatedly(ReturnRef(tradingSystem));

  {
    EXPECT_CALL(*trend, Update(trdk::Price(91.61), point))
        .WillOnce(Return(true));
    const boost::tribool isRising(true);
    EXPECT_CALL(*trend, IsRising()).WillRepeatedly(ReturnRef(isRising));
    const trdk::ScaledPrice askPrice = 87654;
    // To remember close price.
    EXPECT_CALL(tradingSecurity, GetBidPriceScaled())
        .WillRepeatedly(Return(4567));
    // For open price.
    EXPECT_CALL(tradingSecurity, GetAskPriceScaled())
        .WillRepeatedly(Return(askPrice));
    strategy.RaiseLevel1TickEvent(
        spotSecurity, pt::ptime(),
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(9161),
        tms::Milestones());
  }

  {
    EXPECT_CALL(*trend, Update(trdk::Price(92.62), point))
        .WillOnce(Return(true));
    const boost::tribool isRising(false);
    const trdk::ScaledPrice bidPrice = 65478;
    // For open price.
    EXPECT_CALL(tradingSecurity, GetBidPriceScaled())
        .WillRepeatedly(Return(bidPrice));
    // To remember close price.
    EXPECT_CALL(tradingSecurity, GetAskPriceScaled())
        .WillRepeatedly(Return(4567));
    EXPECT_CALL(*trend, IsRising()).WillRepeatedly(ReturnRef(isRising));
    strategy.RaiseLevel1TickEvent(
        spotSecurity, pt::ptime(),
        trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(9262),
        tms::Milestones());
  }
}

TEST(MrigeshKejriwal_Strategy, Trend) {
  mk::Trend trend;
  EXPECT_TRUE(boost::indeterminate(trend.IsRising()));
  {
    // Waiting for 1st signal.
    ASSERT_FALSE(trend.Update(5, 20));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(10, 20));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(20, 30));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
  {
    ASSERT_TRUE(trend.Update(40, 30));
    EXPECT_EQ(boost::tribool(true), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(51, 50));
    EXPECT_EQ(boost::tribool(true), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(51, 50));
    EXPECT_EQ(boost::tribool(true), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(50, 50));
    EXPECT_EQ(boost::tribool(true), trend.IsRising());
  }
  {
    ASSERT_TRUE(trend.Update(49, 50));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
  {
    ASSERT_FALSE(trend.Update(50, 50));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
  {
    ASSERT_TRUE(trend.Update(51, 50));
    EXPECT_EQ(boost::tribool(true), trend.IsRising());
  }
  {
    ASSERT_TRUE(trend.Update(49, 50));
    EXPECT_EQ(boost::tribool(false), trend.IsRising());
  }
}
