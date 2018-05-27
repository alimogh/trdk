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
#include "TradingLib/OrderPolicy.hpp"
#include "TradingLib/PnlContainer.hpp"
#include "TradingLib/PositionController.hpp"
#include "Core/Operation.hpp"
#include "Core/Position.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace uuids = boost::uuids;

////////////////////////////////////////////////////////////////////////////////

namespace trdk {
namespace Strategies {
namespace Test {

////////////////////////////////////////////////////////////////////////////////

class Operation : public trdk::Operation {
 public:
  typedef trdk::Operation Base;

  explicit Operation(Strategy &strategy)
      : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()),
        m_orderPolicy(boost::make_shared<LimitGtcOrderPolicy>()) {}
  ~Operation() override = default;

  const OrderPolicy &GetOpenOrderPolicy(const Position &) const override {
    return *m_orderPolicy;
  }
  const OrderPolicy &GetCloseOrderPolicy(const Position &) const override {
    return *m_orderPolicy;
  }
  bool IsLong(const Security &) const override { return *GetIsRising(); }
  Qty GetPlannedQty(const Security &) const override { return 10; }
  bool HasCloseSignal(const Position &position) const override {
    const auto &isRising = GetIsRising();
    return !isRising || IsLong(position.GetSecurity()) == position.IsLong();
  }
  boost::shared_ptr<trdk::Operation> StartInvertedPosition(
      const Position &) override {
    return boost::make_shared<Operation>(GetStrategy());
  }

 private:
  boost::optional<bool> GetIsRising() const;

  const boost::shared_ptr<OrderPolicy> m_orderPolicy;
};

////////////////////////////////////////////////////////////////////////////////

class TestStrategy : public Strategy {
  friend class Operation;

 public:
  typedef Strategy Super;

  explicit TestStrategy(Context &context,
                        const std::string &name,
                        const std::string &instanceName,
                        const ptr::ptree &conf)
      : Super(context,
              "{063AB9A2-EE3E-4AF7-85B0-AC0B63E27F43}",
              name,
              instanceName,
              conf),
        m_direction(0),
        m_prevPrice(0) {}

  ~TestStrategy() override = default;

 protected:
  void OnPositionUpdate(Position &position) {
    m_positionController.OnUpdate(position);
  }

  void OnLevel1Tick(Security &security,
                    const pt::ptime &,
                    const Level1TickValue &tick,
                    const Milestones &delayMeasurement) override {
    if (tick.GetType() != LEVEL1_TICK_LAST_PRICE) {
      return;
    }

    {
      const auto &lastPrice = tick.GetValue();
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

  void OnPostionsCloseRequest() override {
    m_positionController.OnCloseAllRequest(*this);
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
    GetTradingLog().Write("trend\tchanged\t%1%",
                          [&isRising](TradingRecord &record) {
                            record % (*isRising ? "rising" : "falling");  // 1
                          });
    m_positionController.OnSignal(m_operation, 0, security, delayMeasurement);
  }

 private:
  boost::shared_ptr<Operation> m_operation;
  PositionController m_positionController;
  intmax_t m_direction;
  Price m_prevPrice;
};

////////////////////////////////////////////////////////////////////////////////

boost::optional<bool> Operation::GetIsRising() const {
  return boost::polymorphic_downcast<const TestStrategy *>(&GetStrategy())
      ->GetIsRising();
}

////////////////////////////////////////////////////////////////////////////////
}  // namespace Test
}  // namespace Strategies
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateStrategy(Context &context,
                                           const std::string &instanceName,
                                           const ptr::ptree &conf) {
  return boost::shared_ptr<Strategy>(
      new Strategies::Test::TestStrategy(context, "Test", instanceName, conf));
}

////////////////////////////////////////////////////////////////////////////////
