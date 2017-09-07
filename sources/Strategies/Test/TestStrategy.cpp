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
#include "TradingLib/PositionController.hpp"
#include "Core/Position.hpp"
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

////////////////////////////////////////////////////////////////////////////////

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(Strategy &strategy)
      : Base(strategy),
        m_orderPolicy(boost::make_shared<TradingLib::LimitGtcOrderPolicy>()) {}

  virtual ~PositionController() override = default;

 public:
  virtual Position &OpenPosition(Security &security,
                                 const Milestones &delayMeasurement) override {
    Assert(GetIsRising());
    return OpenPosition(security, *GetIsRising(), delayMeasurement);
  }

  using Base::OpenPosition;

 protected:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override {
    return *m_orderPolicy;
  }
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override {
    return *m_orderPolicy;
  }

  virtual Qty GetNewPositionQty() const override { return 10; }

  virtual bool IsPositionCorrect(const Position &position) const override {
    const auto &isRising = GetIsRising();
    return !isRising || *isRising == position.IsLong();
  }

 private:
  boost::optional<bool> GetIsRising() const;

 private:
  const boost::shared_ptr<TradingLib::OrderPolicy> m_orderPolicy;
};

////////////////////////////////////////////////////////////////////////////////

class TestStrategy : public Strategy {
  friend class PositionController;

 public:
  typedef Strategy Super;

 public:
  explicit TestStrategy(Context &context,
                        const std::string &name,
                        const std::string &instanceName,
                        const Lib::IniSectionRef &conf)
      : Super(context,
              "{063AB9A2-EE3E-4AF7-85B0-AC0B63E27F43}",
              name,
              instanceName,
              conf),
        m_positionController(*this),
        m_direction(0),
        m_prevPrice(0) {}

  virtual ~TestStrategy() override = default;

 protected:
  void OnPositionUpdate(Position &position) {
    m_positionController.OnPositionUpdate(position);
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
    m_positionController.OnPostionsCloseRequest();
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
    m_positionController.OnSignal(security, delayMeasurement);
  }

 private:
  PositionController m_positionController;
  intmax_t m_direction;
  Price m_prevPrice;
};

////////////////////////////////////////////////////////////////////////////////

boost::optional<bool> PositionController::GetIsRising() const {
  return boost::polymorphic_downcast<const TestStrategy *>(&GetStrategy())
      ->GetIsRising();
}

////////////////////////////////////////////////////////////////////////////////
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
