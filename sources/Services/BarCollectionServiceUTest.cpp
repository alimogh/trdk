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
#include "Core/ContextMock.hpp"
#include "Core/DropCopyMock.hpp"
#include "Core/MarketDataSourceMock.hpp"
#include "Core/SecurityMock.hpp"
#include "BarCollectionService.hpp"

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

  explicit Tick(const pt::ptime &time,
                const trdk::ScaledPrice &price,
                const trdk::Qty &qty)
      : time(time), price(price), qty(qty) {}
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
    result.emplace_back(time + pt::milliseconds(timeOffset++), i + 1, i + 2);
  }

  return result;
}
}

////////////////////////////////////////////////////////////////////////////////

TEST(BarServiceTest, ByNumberOfTicks) {
  const lib::IniString settings(
      "[Section]\n"
      "size = 10 ticks\n"
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "log = none");

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());

  svc::BarCollectionService service(context, "Test",
                                    lib::IniSectionRef(settings, "Section"));
  {
    trdk::Security::Request request;
    service.OnSecurityStart(security, request);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(10, request.GetNumberOfTicks());
  }

  const size_t numberOfSets = 39;
  const Source &source = GetSource(numberOfSets);

  double volume = 0;
  for (size_t i = 0, tradesCount = 0; i < source.size(); ++i) {
    const auto &tick = source[i];

    ASSERT_THROW(
        service.OnLevel1Tick(
            security, tick.time,
            trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_PRICE>(
                tick.price)),
        svc::BarCollectionService::MethodDoesNotSupportBySettings);
    ASSERT_THROW(service.OnLevel1Tick(
                     security, tick.time,
                     trdk::Level1TickValue::Create<trdk::LEVEL1_TICK_LAST_QTY>(
                         tick.qty)),
                 svc::BarCollectionService::MethodDoesNotSupportBySettings);

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
      ASSERT_EQ(tick.time, service.GetLastBar().time) << i;
      ASSERT_EQ(source[i + 1 - tradesCount].price,
                service.GetLastBar().openTradePrice);
      ASSERT_EQ(tick.price, service.GetLastBar().closeTradePrice);
      ASSERT_EQ(tick.price, service.GetLastBar().highTradePrice);
      ASSERT_EQ(source[i + 1 - tradesCount].price,
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
        ASSERT_EQ(source[i + 1 - tradesCount - 10].price,
                  service.GetLastBar().openTradePrice);
        ASSERT_EQ(prevTick.price, service.GetLastBar().closeTradePrice);
        ASSERT_EQ(prevTick.price, service.GetLastBar().highTradePrice);
        ASSERT_EQ(source[i + 1 - tradesCount - 10].price,
                  service.GetLastBar().lowTradePrice);

      } else {
        EXPECT_THROW(service.GetLastBar(),
                     svc::BarCollectionService::BarDoesNotExistError);
      }
    }
  }

  {
    EXPECT_TRUE(service.CompleteBar());
    service.Reset();
    EXPECT_TRUE(service.IsEmpty());
    EXPECT_EQ(0, service.GetSize());
    EXPECT_THROW(service.GetLastBar(),
                 svc::BarCollectionService::BarDoesNotExistError);
    EXPECT_THROW(service.GetBar(0),
                 svc::BarCollectionService::BarDoesNotExistError);
    EXPECT_THROW(service.GetBarByReversedIndex(0),
                 svc::BarCollectionService::BarDoesNotExistError);
  }
}

TEST(BarServiceTest, ByNumberOfTicksWithSessionOpenClose) {
  const lib::IniString settings(
      "[Section]\n"
      "size = 10000 ticks\n"
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "log = none");

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());
  svc::BarCollectionService service(context, "Test",
                                    lib::IniSectionRef(settings, "Section"));

  {
    trdk::Security::Request request;
    service.OnSecurityStart(security, request);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(10000, request.GetNumberOfTicks());
  }
  EXPECT_FALSE(service.OnSecurityServiceEvent(
      pt::microsec_clock::local_time(), security,
      trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));

  EXPECT_FALSE(service.OnSecurityServiceEvent(
      pt::microsec_clock::local_time(), security,
      trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));
  EXPECT_EQ(0, service.GetSize());
  EXPECT_THROW(service.GetLastBar(),
               svc::BarCollectionService::BarDoesNotExistError);

  EXPECT_FALSE(
      service.OnNewTrade(security, pt::microsec_clock::local_time(), 1, 1));
  EXPECT_EQ(0, service.GetSize());
  EXPECT_THROW(service.GetLastBar(),
               svc::BarCollectionService::BarDoesNotExistError);
  EXPECT_FALSE(
      service.OnNewTrade(security, pt::microsec_clock::local_time(), 2, 2));
  EXPECT_EQ(0, service.GetSize());
  EXPECT_THROW(service.GetLastBar(),
               svc::BarCollectionService::BarDoesNotExistError);

  EXPECT_TRUE(service.OnSecurityServiceEvent(
      pt::microsec_clock::local_time(), security,
      trdk::Security::SERVICE_EVENT_TRADING_SESSION_CLOSED));
  EXPECT_EQ(1, service.GetSize());
  EXPECT_EQ(1, service.GetLastBar().openTradePrice);
  EXPECT_EQ(2, service.GetLastBar().closeTradePrice);
  EXPECT_EQ(2, service.GetLastBar().highTradePrice);
  EXPECT_EQ(1, service.GetLastBar().lowTradePrice);

  EXPECT_FALSE(
      service.OnNewTrade(security, pt::microsec_clock::local_time(), 3, 3));
  EXPECT_EQ(1, service.GetSize());

  EXPECT_TRUE(service.OnSecurityServiceEvent(
      pt::microsec_clock::local_time(), security,
      trdk::Security::SERVICE_EVENT_TRADING_SESSION_OPENED));
  EXPECT_EQ(2, service.GetSize());
  EXPECT_EQ(3, service.GetLastBar().openTradePrice);
  EXPECT_EQ(3, service.GetLastBar().closeTradePrice);
  EXPECT_EQ(3, service.GetLastBar().highTradePrice);
  EXPECT_EQ(3, service.GetLastBar().lowTradePrice);

  EXPECT_FALSE(
      service.OnNewTrade(security, pt::microsec_clock::local_time(), 4, 4));
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
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "log = none");

  Mocks::Context context;

  MockSecurity security;
  svc::BarCollectionService service(context, "Test",
                                    lib::IniSectionRef(settings, "Section"));

  {
    trdk::Security::Request request;
    service.OnSecurityStart(security, request);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(2, request.GetNumberOfTicks());
  }

  const auto &now = pt::microsec_clock::local_time();
  EXPECT_FALSE(service.OnNewTrade(security, now, 1, 3));
  EXPECT_EQ(0, service.GetSize());
  EXPECT_TRUE(service.IsEmpty());
  EXPECT_TRUE(service.OnNewTrade(security, now, 1, 3));
  EXPECT_FALSE(service.IsEmpty());
  EXPECT_EQ(1, service.GetSize());
  EXPECT_FALSE(service.OnNewTrade(security, now, 1, 3));
  EXPECT_FALSE(service.IsEmpty());
  EXPECT_EQ(1, service.GetSize());
  EXPECT_TRUE(service.OnNewTrade(security, now, 1, 3));
  EXPECT_FALSE(service.IsEmpty());
  EXPECT_EQ(2, service.GetSize());
}

TEST(BarServiceTest, DropCopy) {
  const lib::IniString settings(
      "[Section]\n"
      "size = 3 ticks\n"
      "id = {00000000-0000-0000-0000-000000000001}\n"
      "log = none");

  MockDropCopy dropCopy;

  Mocks::Context context;
  Mocks::MarketDataSource marketDataSource;
  const trdk::Security security(context, lib::Symbol("TEST_SCALE2/USD::STK"),
                                marketDataSource,
                                trdk::Security::SupportedLevel1Types());

  svc::BarCollectionService service(context, "Test",
                                    lib::IniSectionRef(settings, "Section"));
  {
    trdk::Security::Request request;
    service.OnSecurityStart(security, request);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(3, request.GetNumberOfTicks());
  }

  ////////////////////////////////////////////////////////////////////////////////

  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    EXPECT_THROW(service.DropLastBarCopy(12),
                 svc::BarCollectionService::BarDoesNotExistError);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    service.DropUncompletedBarCopy(12);
  }

  ////////////////////////////////////////////////////////////////////////////////

  const auto time1 = pt::microsec_clock::local_time();
  service.OnNewTrade(security, time1, 110, 2);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    EXPECT_THROW(service.DropLastBarCopy(12),
                 svc::BarCollectionService::BarDoesNotExistError);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(12, 0, time1, 1.1, 1.1, 1.1, 1.1)).Times(1);
    service.DropUncompletedBarCopy(12);
  }

  const auto time2 = time1 + pt::seconds(12);
  service.OnNewTrade(security, time2, 330, 4);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    EXPECT_THROW(service.DropLastBarCopy(12),
                 svc::BarCollectionService::BarDoesNotExistError);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(13, 0, time2, 1.1, 3.3, 1.1, 3.3)).Times(1);
    service.DropUncompletedBarCopy(13);
  }

  const auto time3 = time2 + pt::seconds(12);
  service.OnNewTrade(security, time3, 550, 6);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(15, 0, time3, 1.1, 5.5, 1.1, 5.5)).Times(1);
    service.DropLastBarCopy(15);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    service.DropUncompletedBarCopy(15);
  }

  ////////////////////////////////////////////////////////////////////////////////

  const auto time4 = time3 + pt::seconds(43);
  service.OnNewTrade(security, time4, 110, 99);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(15, 0, time3, 1.1, 5.5, 1.1, 5.5)).Times(1);
    service.DropLastBarCopy(15);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(12, 1, time4, 1.1, 1.1, 1.1, 1.1)).Times(1);
    service.DropUncompletedBarCopy(12);
  }

  const auto time5 = time4 + pt::seconds(12);
  service.OnNewTrade(security, time5, 330, 4);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(15, 0, time3, 1.1, 5.5, 1.1, 5.5)).Times(1);
    service.DropLastBarCopy(15);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(13, 1, time5, 1.1, 3.3, 1.1, 3.3)).Times(1);
    service.DropUncompletedBarCopy(13);
  }

  const auto time6 = time5 + pt::seconds(12);
  service.OnNewTrade(security, time6, 550, 6);
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(1).WillOnce(Return(&dropCopy));
    EXPECT_CALL(dropCopy, CopyBar(15, 1, time6, 1.1, 5.5, 1.1, 5.5)).Times(1);
    service.DropLastBarCopy(15);
  }
  {
    InSequence s;
    EXPECT_CALL(context, GetDropCopy()).Times(0);
    service.DropUncompletedBarCopy(15);
  }

  ////////////////////////////////////////////////////////////////////////////////
}
