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
using namespace trdk::TradingLib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

aa::Operation::Operation(
    trdk::Strategy &strategy,
    Security &sellTarget,
    Security &buyTarget,
    const Qty &maxQty,
    const Price &sellPrice,
    const Price &buyPrice,
    const boost::optional<pt::time_duration> &stopLossDelay)
    : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()),
      m_openOrderPolicy(sellPrice, buyPrice),
      m_sellTarget(sellTarget),
      m_buyTarget(buyTarget),
      m_maxQty(maxQty),
      m_stopLossDelay(stopLossDelay) {}

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

  class StopLoss : public TradingLib::StopLossOrder {
   public:
    typedef TradingLib::StopLossOrder Base;

   public:
    explicit StopLoss(const Price &controlPrice,
                      Position &position,
                      PositionController &controller,
                      const pt::time_duration &stopLossDelay)
        : Base(stopLossDelay, position, controller),
          m_controlPrice(controlPrice) {}
    virtual ~StopLoss() override = default;

   public:
    virtual void Report(const char *action) const override {
      GetTradingLog().Write(
          "{'algo': {'action': '%6%', 'type': '%1%', 'params': {'price': "
          "'%7% %2$.8f'}, 'delayTime': '%3%', 'position': {'side': '%8%', "
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

    virtual bool IsWatching() const override {
      if (!GetPosition().HasOpenedOpenOrders()) {
        return false;
      }
      {
        const auto *const oppositePosition =
            FindOppositePosition(GetPosition());
        if (oppositePosition && oppositePosition->HasActiveOpenOrders() &&
            !oppositePosition->HasOpenedOpenOrders()) {
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
    virtual const char *GetName() const override {
      return "opening stop price";
    }
    virtual const pt::ptime &GetStartTime() const override {
      Assert(m_startTime != pt::not_a_date_time);
      return m_startTime;
    }

    virtual bool Activate() override {
      const auto &currentPrice = GetPosition().GetMarketOpenPrice();
      if (GetPosition().IsLong()) {
        if (currentPrice <= m_controlPrice) {
          return false;
        }
      } else if (m_controlPrice <= currentPrice) {
        return false;
      }

      GetTradingLog().Write(
          "{'algo': {'action': 'hit', 'type': '%1%', 'price': '%2$.8f %3% "
          "%4$.8f', 'bid': %5$.8f, 'ask': %6$.8f, 'position': {'side': '%9%', "
          "'operation': '%7%/%8%'}}}",
          [this, &currentPrice](TradingRecord &record) {
            record % GetName()                                    // 1
                % currentPrice                                    // 2
                % (GetPosition().IsLong() ? ">" : "<")            // 3
                % m_controlPrice                                  // 4
                % GetPosition().GetSecurity().GetBidPriceValue()  // 5
                % GetPosition().GetSecurity().GetAskPriceValue()  // 6
                % GetPosition().GetOperation()->GetId()           // 7
                % GetPosition().GetSubOperationId()               // 8
                % GetPosition().GetSide();                        // 9
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
