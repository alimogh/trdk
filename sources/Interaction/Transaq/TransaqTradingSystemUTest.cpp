/**************************************************************************
 *   Created: 2017/01/21 15:09:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/ContextMock.hpp"
#include "Core/SecurityMock.hpp"
#include "TransaqConnectorContext.hpp"
#include "TransaqTradingSystem.hpp"

namespace ptr = boost::property_tree;
namespace lib = trdk::Lib;
namespace trq = trdk::Interaction::Transaq;

using namespace trdk::Tests;
using namespace testing;

namespace {

////////////////////////////////////////////////////////////////////////////////

class MockConnectorContext : public trq::ConnectorContext {
 public:
  virtual ~MockConnectorContext() {}

 public:
  MOCK_CONST_METHOD1(SubscribeToNewData,
                     boost::signals2::scoped_connection(const NewDataSlot &));

  virtual ResultPtr SendCommand(
      const char *data,
      const lib::TimeMeasurement::Milestones &milestones) override {
    const auto &dummyDeleter = [](const char *) {

    };
    return ResultPtr(SendCommandProxy(data, milestones), dummyDeleter);
  }

  MOCK_METHOD2(SendCommandProxy,
               const char *(const char *,
                            const lib::TimeMeasurement::Milestones &));
};

////////////////////////////////////////////////////////////////////////////////

class TradingSystem : public trq::TradingSystem {
 public:
  typedef trq::TradingSystem Base;

 public:
  explicit TradingSystem(trdk::Context &context,
                         const lib::IniSectionRef &conf,
                         trq::ConnectorContext &connector)
      : Base(trdk::TRADING_MODE_LIVE, 0, context, "Test", conf),
        m_connector(connector) {}
  virtual ~TradingSystem() {}

 public:
  using Base::SendBuy;
  using Base::SendSell;
  using Base::SendCancelOrder;
  using Base::OnOrderUpdate;
  using Base::OnTrade;

 protected:
  virtual trq::ConnectorContext &GetConnectorContext() override {
    return m_connector;
  }

 private:
  trq::ConnectorContext &m_connector;
};

////////////////////////////////////////////////////////////////////////////////
}

namespace {

//! Tests caces when TRANSAQ server sends trades after order canceling.
/** @sa https://app.asana.com/0/187185900270867/251220194156084
  */
class TransaqTradingSystemTest : public testing::Test {
 protected:
  struct OrderUpdate {
    trdk::OrderId orderId;
    std::string tradingSystemOrderId;
    trdk::OrderStatus status;
    trdk::Qty remainingQty;
    boost::optional<TradingSystem::TradeInfo> tradeInfo;
  };

 protected:
  TransaqTradingSystemTest()
      : m_settings(
            "[Section]\n"
            "rqdelay = 3333\n"
            "session_timeout = 222\n"
            "request_timeout = 111\n"
            "login = xxxxx\n"
            "password = xxxx\n"
            "client = xxxx\n"
            "host = xxx\n"
            "port = 111"),
        m_settingsSection(m_settings, "Section"),
        m_orderUpdateCallback([this](
            const trdk::OrderId &orderId,
            const std::string &tradingSystemOrderId,
            const trdk::OrderStatus &status,
            const trdk::Qty &remainingQty,
            const TradingSystem::TradeInfo *tradeInfo) {
          boost::optional<TradingSystem::TradeInfo> tradeInfoCopy;
          if (tradeInfo) {
            tradeInfoCopy = *tradeInfo;
          }
          m_orderUpdates.emplace_back(OrderUpdate{orderId, tradingSystemOrderId,
                                                  status, remainingQty,
                                                  std::move(tradeInfoCopy)});
        }),
        m_orderCommandResult(CreateOrderCommandResult()) {}

 protected:
  virtual void SetUp() override { m_orderUpdates.clear(); }

  void TestUnknownOrderCancel(TradingSystem &ts) {
    SCOPED_TRACE("Unknown order canceling");

    for (size_t i = 0; i < 100; ++i) {
      ASSERT_THROW(ts.SendCancelOrder(999),
                   TradingSystem::UnknownOrderCancelError);
    }
    for (size_t i = 0; i < 100; ++i) {
      try {
        ts.SendCancelOrder(999);
      } catch (const TradingSystem::UnknownOrderCancelError &) {
        ASSERT_TRUE(false);
      } catch (const TradingSystem::Error &) {
      } catch (...) {
        ASSERT_TRUE(false);
      }
    }

    for (size_t i = 0; i < 99; ++i) {
      ASSERT_THROW(ts.SendCancelOrder(1000 + i),
                   TradingSystem::UnknownOrderCancelError);
    }
    for (size_t i = 0; i < 100; ++i) {
      try {
        ts.SendCancelOrder(999);
      } catch (const TradingSystem::UnknownOrderCancelError &) {
        ASSERT_TRUE(false);
      } catch (const TradingSystem::Error &) {
      } catch (...) {
        ASSERT_TRUE(false);
      }
    }
  }

 private:
  const char *CreateOrderCommandResult() {
    {
      std::ostringstream os;
      ptr::ptree orderCommandResult;
      {
        ptr::ptree result;
        result.put("<xmlattr>.transactionid", 999);
        result.put("<xmlattr>.success", true);
        orderCommandResult.add_child("result", result);
      }
      ptr::write_xml(os, orderCommandResult);
      const_cast<std::string &>(m_orderCommandResultBuffer) = os.str();
    }
    TrdkAssert(
        boost::starts_with(m_orderCommandResultBuffer,
                           "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"));
    AssertLe(39, m_orderCommandResultBuffer.size());
    return m_orderCommandResultBuffer.c_str() + 39;
  }

 protected:
  const lib::IniString m_settings;
  const lib::IniSectionRef m_settingsSection;

  std::vector<OrderUpdate> m_orderUpdates;
  TradingSystem::OrderStatusUpdateSlot m_orderUpdateCallback;

  const std::string m_orderCommandResultBuffer;
  const char *const m_orderCommandResult;

  Mocks::Context m_context;
  MockConnectorContext m_connector;
  MockSecurity m_security;
};
}

TEST_F(TransaqTradingSystemTest, CanceledWithoutTrades) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  const auto &orderId =
      ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                 TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
  ASSERT_EQ(999, orderId);
  ASSERT_EQ(0, m_orderUpdates.size());

  ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                   "tradingSystemMessage",
                   lib::TimeMeasurement::Milestones::TimePoint());
  ASSERT_EQ(1, m_orderUpdates.size());
  EXPECT_EQ(999, m_orderUpdates.back().orderId);
  EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
  EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
  EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
  EXPECT_FALSE(m_orderUpdates.back().tradeInfo);

  m_orderUpdates.clear();
  ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 11,
                   "tradingSystemMessage",
                   lib::TimeMeasurement::Milestones::TimePoint());
  ASSERT_EQ(1, m_orderUpdates.size());
  EXPECT_EQ(999, m_orderUpdates.back().orderId);
  EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
  EXPECT_EQ(trdk::ORDER_STATUS_CANCELLED, m_orderUpdates.back().status);
  EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
  EXPECT_FALSE(m_orderUpdates.back().tradeInfo);

  m_orderUpdates.clear();
  ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 0,
                   "tradingSystemMessage",
                   lib::TimeMeasurement::Milestones::TimePoint());
  ASSERT_EQ(0, m_orderUpdates.size());

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, CanceledAfter2Trades) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  const auto &orderId =
      ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                 TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
  ASSERT_EQ(999, orderId);
  ASSERT_EQ(0, m_orderUpdates.size());

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 5,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_CANCELLED, m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 1,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, CanceledBefore1TradeAndAfter1Trade) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  const auto &orderId =
      ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                 TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
  ASSERT_EQ(999, orderId);
  ASSERT_EQ(0, m_orderUpdates.size());

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 5,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_CANCELLED, m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 1,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, CanceledBefore2Trades) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  const auto &orderId =
      ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                 TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
  ASSERT_EQ(999, orderId);
  ASSERT_EQ(0, m_orderUpdates.size());

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 5,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_CANCELLED, m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_CANCELLED, 1,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, FilledWith1FullTradeBefore) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  {
    const auto &orderId =
        ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                   TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
    ASSERT_EQ(999, orderId);
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer", 123134234, 665, 11);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED, m_orderUpdates.back().status);
    EXPECT_EQ(0, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(11, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_FILLED, 0,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, FilledWith4FullTradesBefore) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  {
    const auto &orderId =
        ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                   TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
    ASSERT_EQ(999, orderId);
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer3", 123134234, 665, 4);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(1, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer3", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(4, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer4", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED, m_orderUpdates.back().status);
    EXPECT_EQ(0, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer4", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_FILLED, 0,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, FilledWith3TradesBeforeAnd1After) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  {
    const auto &orderId =
        ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                   TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
    ASSERT_EQ(999, orderId);
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer3", 123134234, 665, 4);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(1, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer3", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(4, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_FILLED, 0,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer4", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED, m_orderUpdates.back().status);
    EXPECT_EQ(0, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer4", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, FilledWith2TradesBeforeAnd2After) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  {
    const auto &orderId =
        ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                   TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
    ASSERT_EQ(999, orderId);
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_FILLED, 0,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer3", 123134234, 665, 4);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(1, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer3", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(4, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer4", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED, m_orderUpdates.back().status);
    EXPECT_EQ(0, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer4", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  TestUnknownOrderCancel(ts);
}

TEST_F(TransaqTradingSystemTest, FilledWith4TradesAfter) {
  TradingSystem ts(m_context, m_settingsSection, m_connector);

  EXPECT_CALL(m_connector, SendCommandProxy(_, _))
      .WillOnce(Return(m_orderCommandResult));

  {
    const auto &orderId =
        ts.SendBuy(m_security, lib::CURRENCY_USD, 11, 123, trdk::OrderParams{},
                   TradingSystem::OrderStatusUpdateSlot(m_orderUpdateCallback));
    ASSERT_EQ(999, orderId);
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_SUBMITTED, 11,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_SUBMITTED, m_orderUpdates.back().status);
    EXPECT_EQ(11, m_orderUpdates.back().remainingQty);
    EXPECT_FALSE(m_orderUpdates.back().tradeInfo);
  }

  {
    m_orderUpdates.clear();
    ts.OnOrderUpdate(999, 123134234, trdk::ORDER_STATUS_FILLED, 0,
                     "tradingSystemMessage",
                     lib::TimeMeasurement::Milestones::TimePoint());
    ASSERT_EQ(0, m_orderUpdates.size());
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer1", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(10, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer1", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer2", 123134234, 665, 5);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(5, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer2", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(5, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer3", 123134234, 665, 4);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED_PARTIALLY,
              m_orderUpdates.back().status);
    EXPECT_EQ(1, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer3", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(4, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  {
    m_orderUpdates.clear();
    ts.OnTrade("sfgaer4", 123134234, 665, 1);
    ASSERT_EQ(1, m_orderUpdates.size());
    EXPECT_EQ(999, m_orderUpdates.back().orderId);
    EXPECT_EQ("123134234", m_orderUpdates.back().tradingSystemOrderId);
    EXPECT_EQ(trdk::ORDER_STATUS_FILLED, m_orderUpdates.back().status);
    EXPECT_EQ(0, m_orderUpdates.back().remainingQty);
    ASSERT_TRUE(m_orderUpdates.back().tradeInfo);
    EXPECT_EQ("sfgaer4", m_orderUpdates.back().tradeInfo->id);
    EXPECT_EQ(1, m_orderUpdates.back().tradeInfo->qty);
    EXPECT_EQ(66500, m_orderUpdates.back().tradeInfo->price);
  }

  TestUnknownOrderCancel(ts);
}
