/**************************************************************************
 *   Created: 2014/08/07 12:39:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Position.hpp"
#include "Core/Security.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Api.h"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;

////////////////////////////////////////////////////////////////////////////////

namespace trdk {
namespace Strategies {
namespace Test {

class TestStrategy : public Strategy {
 public:
  typedef Strategy Super;

 public:
  explicit TestStrategy(Context &context,
                        const std::string &name,
                        const std::string &instanceName,
                        const Lib::IniSectionRef &conf)
      : Super(
            context,
            uuids::string_generator()("{063AB9A2-EE3E-4AF7-85B0-AC0B63E27F43}"),
            name,
            instanceName,
            conf),
        m_direction(0),
        m_prevPrice(0) {}

  virtual ~TestStrategy() override = default;

 protected:
  void OnPositionUpdate(Position &position) {
    AssertLt(0, position.GetNumberOfOpenOrders());

    if (position.IsCompleted()) {
      // No active order, no active qty...

      Assert(!position.HasActiveOrders());
      const auto &isRising = GetIsRising();
      if (position.GetNumberOfCloseOrders()) {
        // Position fully closed.
        ReportOperation(position);
        if (isRising && *isRising != position.IsLong()) {
          // Opening opposite position.
          OpenPosition(position.GetSecurity(), !position.IsLong(),
                       Milestones());
        }
        return;
      }
      // Open order was canceled by some condition. Checking open
      // signal again and sending new open order...
      if (isRising && *isRising == position.IsLong()) {
        ContinuePosition(position);
      }

      // Position will be deleted if was not continued.

    } else if (position.GetNumberOfCloseOrders()) {
      // Position closing started.

      Assert(!position.HasActiveOpenOrders());
      AssertNe(CLOSE_REASON_NONE, position.GetCloseReason());

      if (position.HasActiveCloseOrders()) {
        // Closing in progress.
      } else if (position.GetCloseReason() != CLOSE_REASON_NONE) {
        // Close order was canceled by some condition. Sending
        // new close order.
        ClosePosition(position);
      }

    } else if (position.HasActiveOrders()) {
      // Opening in progress.

      Assert(!position.HasActiveCloseOrders());

      const auto &isRising = GetIsRising();
      if (isRising && *isRising != position.IsLong()) {
        // Close signal received, closing position...
        ClosePosition(position);
      }

    } else {
      // Position is active.

      const auto &isRising = GetIsRising();
      if (isRising) {
        if (*isRising == position.IsLong()) {
          // Holding position and waiting for close signal...
          AssertLt(0, position.GetActiveQty());
        } else {
          // Holding position and received close signal...
          AssertLt(0, position.GetActiveQty());
          ClosePosition(position);
        }
      }
    }
  }

  virtual void OnLevel1Tick(Security &security,
                            const pt::ptime &,
                            const Level1TickValue &tick,
                            const Milestones &delayMeasurement) override {
    if (tick.GetType() != LEVEL1_TICK_LAST_PRICE) {
      return;
    }

    {
      const auto &lastPrice = security.DescalePrice(tick.GetValue());
      if (m_prevPrice < lastPrice) {
        if (m_direction < 0) {
          m_direction = 1;
        } else {
          ++m_direction;
        }
      } else if (m_prevPrice > lastPrice) {
        if (m_direction > 0) {
          m_direction = -1;
        } else {
          --m_direction;
        }
      }
      m_prevPrice = std::move(lastPrice);
    }

    CheckSignal(security, delayMeasurement);
  }

  virtual void OnPostionsCloseRequest() override {
    throw MethodDoesNotImplementedError(
        "trdk::Strategies::Test::TestStrategy"
        "::OnPostionsCloseRequest is not implemented");
  }

 private:
  boost::optional<bool> GetIsRising() const {
    if (abs(m_direction) < 3) {
      return boost::none;
    }
    return m_direction > 0;
  }

  void CheckSignal(Security &security, const Milestones &delayMeasurement) {
    const auto &isRising = GetIsRising();
    if (!isRising) {
      return;
    }

    Position *position = nullptr;
    if (!GetPositions().IsEmpty()) {
      AssertEq(1, GetPositions().GetSize());
      position = &*GetPositions().GetBegin();
      if (position->IsCompleted()) {
        position = nullptr;
      } else if (*isRising == position->IsLong()) {
        delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
        return;
      }
    }

    GetTradingLog().Write("trend\tchanged\t%1%",
                          [&isRising](TradingRecord &record) {
                            record % (*isRising ? "rising" : "falling");  // 1
                          });

    if (position) {
      if (position->IsCancelling() || position->HasActiveCloseOrders()) {
        return;
      }
    }

    delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

    if (!position) {
      OpenPosition(security, *isRising, delayMeasurement);
    } else if (position->HasActiveOpenOrders()) {
      try {
        Verify(position->CancelAllOrders());
      } catch (const TradingSystem::UnknownOrderCancelError &ex) {
        GetTradingLog().Write("failed to cancel order");
        GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
        return;
      }
    } else {
      ClosePosition(*position);
    }

    delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);
  }

  void OpenPosition(Security &security,
                    bool isLong,
                    const Milestones &delayMeasurement) {
    auto position =
        isLong ? CreatePosition<LongPosition>(
                     security, security.GetAskPriceScaled(), delayMeasurement)
               : CreatePosition<ShortPosition>(
                     security, security.GetBidPriceScaled(), delayMeasurement);
    ContinuePosition(*position);
  }

  template <typename PositionType>
  boost::shared_ptr<Position> CreatePosition(
      Security &security,
      const ScaledPrice &price,
      const Milestones &delayMeasurement) {
    return boost::make_shared<PositionType>(
        *this, m_generateUuid(), 1,
        GetTradingSystem(security.GetSource().GetIndex()), security,
        security.GetSymbol().GetCurrency(), 1, price, delayMeasurement);
  }

  void ContinuePosition(Position &position) {
    Assert(!position.HasActiveOrders());
    position.Open(position.GetMarketOpenPrice());
  }

  void ClosePosition(Position &position) {
    Assert(!position.HasActiveOrders());
    position.SetCloseReason(CLOSE_REASON_SIGNAL);
    position.Close(position.GetMarketClosePrice());
  }

  void ReportOperation(const Position &pos) {
    if (!m_strategyLog.is_open()) {
      m_strategyLog = OpenDataLog("csv");
      m_strategyLog << "Date,Open Start Time,Open Time,Opening Duration"
                    << ",Close Start Time,Close Time,Closing Duration"
                    << ",Position Duration,Type,P&L Volume,P&L %,Is Profit"
                    << ",Is Loss,Qty,Open Price,Open Orders,Open Trades"
                    << ",Close Reason,Close Price,Close Orders, Close Trades,ID"
                    << std::endl;
    }

    const auto &security = pos.GetSecurity();

    m_strategyLog
        << pos.GetOpenStartTime().date() << ','
        << ExcelTextField(pos.GetOpenStartTime().time_of_day()) << ','
        << ExcelTextField(pos.GetOpenTime().time_of_day()) << ','
        << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime()) << ','
        << ExcelTextField(pos.GetCloseStartTime().time_of_day()) << ','
        << ExcelTextField(pos.GetCloseTime().time_of_day()) << ','
        << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime()) << ','
        << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime()) << ','
        << pos.GetType() << ',' << pos.GetRealizedPnlVolume() << ','
        << pos.GetRealizedPnlRatio() << (pos.IsProfit() ? ",1,0" : ",0,1")
        << ',' << pos.GetOpenedQty() << ','
        << security.DescalePrice(pos.GetOpenAvgPrice()) << ','
        << pos.GetNumberOfOpenOrders() << ',' << pos.GetNumberOfOpenTrades()
        << ',' << pos.GetCloseReason() << ','
        << security.DescalePrice(pos.GetCloseAvgPrice()) << ','
        << pos.GetNumberOfCloseOrders() << ',' << pos.GetNumberOfCloseTrades()
        << ',' << pos.GetId() << std::endl;
  }

 private:
  boost::uuids::random_generator m_generateUuid;
  std::ofstream m_strategyLog;

  intmax_t m_direction;
  Price m_prevPrice;
};
}
}
}

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_TEST_API boost::shared_ptr<trdk::Strategy> CreateStrategy(
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return boost::shared_ptr<trdk::Strategy>(
      new trdk::Strategies::Test::TestStrategy(context, "Test", instanceName,
                                               conf));
}

////////////////////////////////////////////////////////////////////////////////
