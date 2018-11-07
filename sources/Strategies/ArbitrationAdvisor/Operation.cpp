/*******************************************************************************
 *   Created: 2017/12/20 21:49:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Operation.hpp"

using namespace trdk;
using namespace TradingLib;
using namespace Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace aa = Strategies::ArbitrageAdvisor;

aa::Operation::Operation(trdk::Strategy &strategy,
                         Security &sellTarget,
                         Security &buyTarget,
                         const Qty &maxQty,
                         const Price &sellPrice,
                         const Price &buyPrice,
                         boost::optional<pt::time_duration> stopLossDelay)
    : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()),
      m_openOrderPolicy(sellPrice, buyPrice),
      m_sellTarget(sellTarget),
      m_buyTarget(buyTarget),
      m_maxQty(maxQty),
      m_stopLossDelay(std::move(stopLossDelay)) {}

bool aa::Operation::IsSame(const Security &sellTarget,
                           const Security &buyTarget) const {
  return &m_sellTarget == &sellTarget && &m_buyTarget == &buyTarget;
}

void aa::Operation::Setup(Position &position,
                          PositionController &controller) const {
  Base::Setup(position, controller);

  if (!m_stopLossDelay) {
    return;
  }

  class StopLoss : public StopLossOrder {
   public:
    typedef StopLossOrder Base;

    explicit StopLoss(const Price &controlPrice,
                      Position &position,
                      PositionController &controller,
                      const pt::time_duration &stopLossDelay)
        : Base(stopLossDelay, position, controller),
          m_controlPrice(controlPrice) {}
    StopLoss(StopLoss &&) = default;
    StopLoss(const StopLoss &) = delete;
    StopLoss &operator=(StopLoss &&) = delete;
    StopLoss &operator=(const StopLoss &) = delete;
    ~StopLoss() override = default;

    void Report(const char *action) const override {
      GetTradingLog().Write(
          "{'algo': {'action': '%6%', 'type': '%1%', 'params': {'price': "
          "'%7% %2%'}, 'delayTime': '%3%', 'position': {'side': '%8%', "
          "'operation': '%4%/%5%'}}}",
          [this, action](TradingRecord &record) {
            record % GetName()                           // 1
                % m_controlPrice                         // 2
                % GetDelay()                             // 3
                % GetPosition().GetOperation()->GetId()  // 4
                % GetPosition().GetSubOperationId()      // 5
                % action                                 // 6
                % (GetPosition().IsLong() ? ">" : "<")   // 7
                % GetPosition().GetSide();               // 8
          });
    }

    bool IsWatching() const override {
      if (!GetPosition().HasOpenedOpenOrders()) {
        return false;
      }
      {
        const auto *const oppositePosition =
            FindOppositePosition(GetPosition());
        if (oppositePosition && oppositePosition->HasActiveOpenOrders() &&
            !oppositePosition->HasOpenedOpenOrders()) {
          // Opposite position sent orders, but engine doesn't know - is it
          // placed and waits for execution or already filled:
          // HasActiveOpenOrders - sent or opened, HasOpenedOpenOrders - only
          // opened. If check only HasOpenedOpenOrders - we will lose case when
          // it sent, but not confirmed.
          return false;
        }
      }
      if (m_startTime == pt::not_a_date_time) {
        // The first call after the position sent orders.
        m_startTime = GetPosition().GetStrategy().GetContext().GetCurrentTime();
      }
      return true;
    }

   protected:
    const char *GetName() const override { return "opening stop price"; }
    const pt::ptime &GetStartTime() const override {
      Assert(m_startTime != pt::not_a_date_time);
      return m_startTime;
    }

    bool Activate() override {
      const auto &currentPrice = GetPosition().GetMarketOpenPrice();
      const auto &now =
          GetPosition().GetStrategy().GetContext().GetCurrentTime();
      const auto &liveTime = now - GetStartTime();

      const char *reason;
      if (liveTime > pt::minutes(15)) {
        reason = "time";
      } else {
        if (GetPosition().IsLong()) {
          if (currentPrice <= m_controlPrice) {
            return false;
          }
        } else if (m_controlPrice <= currentPrice) {
          return false;
        }
        reason = "price";
      }

      GetTradingLog().Write(
          "{'algo': {'action': 'hit', 'type': '%1%-%10%', 'price': '%2% %3% "
          "%4%', 'liveTime': '%11%', 'bid': %5%, 'ask': %6%, 'position': "
          "{'side': '%9%', 'operation': '%7%/%8%'}}}",
          [this, &currentPrice, &reason](TradingRecord &record) {
            record % GetName()                                    // 1
                % currentPrice                                    // 2
                % (GetPosition().IsLong() ? ">" : "<")            // 3
                % m_controlPrice                                  // 4
                % GetPosition().GetSecurity().GetBidPriceValue()  // 5
                % GetPosition().GetSecurity().GetAskPriceValue()  // 6
                % GetPosition().GetOperation()->GetId()           // 7
                % GetPosition().GetSubOperationId()               // 8
                % GetPosition().GetSide()                         // 9
                % reason                                          // 10
                % GetStartTime();                                 // 11
          });

      return true;
    }

   private:
    const Price m_controlPrice;
    mutable pt::ptime m_startTime;
  };

  position.AddAlgo(boost::make_unique<StopLoss>(
      m_openOrderPolicy.GetOpenOrderPrice(position), position, controller,
      *m_stopLossDelay));
}
