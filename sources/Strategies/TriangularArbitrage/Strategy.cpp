/*******************************************************************************
 *   Created: 2018/03/06 10:02:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "Controller.hpp"
#include "Operation.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::TriangularArbitrage;

namespace ta = trdk::Strategies::TriangularArbitrage;
namespace sig = boost::signals2;

////////////////////////////////////////////////////////////////////////////////

namespace {
class BuyLegPolicy : public LegPolicy {
 public:
  explicit BuyLegPolicy(const std::string &symbol) : LegPolicy(symbol) {}
  virtual ~BuyLegPolicy() override = default;

 public:
  virtual const OrderSide &GetSide() const override {
    static const OrderSide result = ORDER_SIDE_BUY;
    return result;
  }

  virtual Qty GetQty(const Security &security) const override {
    return security.GetAskQty();
  }

  virtual Price GetPrice(const Security &security) const override {
    return security.GetAskPrice();
  }

  virtual Qty GetOrderQtyAllowedByBalance(const TradingSystem &tradingSystem,
                                          const Security &security,
                                          const Price &price) const override {
    auto balance = tradingSystem.GetBalances().GetAvailableToTrade(
        security.GetSymbol().GetQuoteSymbol());
    balance -= tradingSystem.CalcCommission(balance / price, price,
                                            ORDER_SIDE_BUY, security);
    return balance / price;
  }

  virtual Qty CalcPnl(const Qty &thisLegQty, const Qty &leg1Qty) const {
    return thisLegQty - leg1Qty;
  }
};

class SellLegPolicy : public LegPolicy {
 public:
  explicit SellLegPolicy(const std::string &symbol) : LegPolicy(symbol) {}
  virtual ~SellLegPolicy() override = default;

 public:
  virtual const OrderSide &GetSide() const override {
    static const OrderSide result = ORDER_SIDE_SELL;
    return result;
  }

  virtual Qty GetQty(const Security &security) const override {
    return security.GetBidQty();
  }

  virtual Price GetPrice(const Security &security) const override {
    return security.GetBidPrice();
  }

  virtual Qty GetOrderQtyAllowedByBalance(const TradingSystem &tradingSystem,
                                          const Security &security,
                                          const Price &) const override {
    return tradingSystem.GetBalances().GetAvailableToTrade(
        security.GetSymbol().GetBaseSymbol());
  }

  virtual Qty CalcPnl(const Qty &thisLegQty, const Qty &leg1Qty) const {
    return leg1Qty - thisLegQty;
  }
};

template <typename Base>
class DirectLegPolicy : public Base {
 public:
  explicit DirectLegPolicy(const std::string &symbol) : Base(symbol) {}
  virtual ~DirectLegPolicy() override = default;

 public:
  virtual typename Base::X CalcX(const Security &security) const override {
    return GetPrice(security);
  }
};

template <typename Base>
class OppositeLegPolicy : public Base {
 public:
  explicit OppositeLegPolicy(const std::string &symbol) : Base(symbol) {}
  virtual ~OppositeLegPolicy() override = default;

 public:
  virtual typename Base::X CalcX(const Security &security) const override {
    return 1 / GetPrice(security);
  }
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

enum CalcQtysPpoints {
  CALC_QTYS_POINT_LEG_1_ORIGINAL,
  CALC_QTYS_POINT_LEG_1_FORCED,
  CALC_QTYS_POINT_LEG_1_BY_BALANCE,
  CALC_QTYS_POINT_LEG_2_ORIGINAL,
  CALC_QTYS_POINT_LEG_3_ORIGINAL,
  CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3_SOURCE_VOLUME,
  CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3_SOURCE_PRICE,
  CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3,
  CALC_QTYS_POINT_LEG_1_REDUCED_BY_LEG_2_SOURCE_PRICE,
  CALC_QTYS_POINT_LEG_1_REDUCED_BY_LEG_2,
  numberOfCalcQtysPpoints
};

boost::optional<std::string> CheckCalcs(const Opportunity &opportunity) {
  if (opportunity.pnlVolume.IsNan()) {
    return boost::none;
  }
  const auto &targets = opportunity.targets;
  const auto plannedLeg3Qty = targets[LEG_1].qty * opportunity.pnlRatio;
  if (targets[LEG_3].qty < plannedLeg3Qty * 0.75 ||
      plannedLeg3Qty * 1.25 < targets[LEG_3].qty) {
    boost::format error(
        "Legs configuration is wrong - 3rd leg quantity is %1%, but shouldbe "
        "near %2% (P&L ratio is %3%)");
    error % targets[LEG_3].qty   // 1
        % plannedLeg3Qty         // 2
        % opportunity.pnlRatio;  // 3
    return error.str();
  }
  return boost::none;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

class ta::Strategy::Implementation : private boost::noncopyable {
 public:
  ta::Strategy &m_self;

  bool m_isStopped;

  boost::array<std::unique_ptr<LegPolicy>, numberOfLegs> m_legs;
  boost::function<LegPolicy::X(const Opportunity::Targets &)> m_calcPnlRatio;

  size_t m_numberOfSecuriries;

  sig::signal<void(const std::vector<Opportunity> &)> m_opportunitySignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  bool m_isTradingEnabled;
  Volume m_minVolume;
  Volume m_maxVolume;
  Double m_minProfitRatio;
  boost::unordered_set<size_t> m_startExchanges;
  boost::unordered_set<size_t> m_middleExchanges;
  boost::unordered_set<size_t> m_finishExchanges;

  Controller m_controller;

  boost::unordered_set<const TradingSystem *> m_failedTargets;

 public:
  Implementation(ta::Strategy &self)
      : m_self(self), m_isStopped(false), m_numberOfSecuriries(0) {}

  void CheckSignal(const Milestones &delayMeasurement) {
    std::vector<Opportunity> opportunities;
    opportunities.reserve(m_numberOfSecuriries);
    size_t hasSkippedSecurities = 0;
    boost::optional<std::string> configurationError;

    for (auto *const leg1 : m_legs[LEG_1]->GetSecurities()) {
      if (!IsExchangeEnabled(*leg1, m_startExchanges)) {
        hasSkippedSecurities = true;
        continue;
      }
      auto &leg1Trading = m_self.GetTradingSystem(leg1->GetSource().GetIndex());
      for (auto *const leg2 : m_legs[LEG_2]->GetSecurities()) {
        if (!IsExchangeEnabled(*leg2, m_middleExchanges)) {
          hasSkippedSecurities = true;
          continue;
        }
        auto &leg2Trading =
            m_self.GetTradingSystem(leg2->GetSource().GetIndex());
        for (auto *const leg3 : m_legs[LEG_3]->GetSecurities()) {
          if (!IsExchangeEnabled(*leg3, m_finishExchanges)) {
            hasSkippedSecurities = true;
            continue;
          }
          auto &leg3Trading =
              m_self.GetTradingSystem(leg3->GetSource().GetIndex());
          try {
            const auto thisConfigurationError = CheckSignal(
                opportunities,
                {Opportunity::Target{leg1, &leg1Trading,
                                     m_legs[LEG_1]->GetPrice(*leg1)},
                 Opportunity::Target{leg2, &leg2Trading,
                                     m_legs[LEG_2]->GetPrice(*leg2)},
                 Opportunity::Target{leg3, &leg3Trading,
                                     m_legs[LEG_3]->GetPrice(*leg3)}});
            if (thisConfigurationError && !configurationError) {
              configurationError = std::move(thisConfigurationError);
            }
          } catch (const Security::MarketDataValueDoesNotExist &) {
            hasSkippedSecurities = true;
          }
        }
      }
    }

    if (opportunities.empty() && !hasSkippedSecurities) {
      throw Exception("One or more legs don't have securities");
    }

    std::sort(opportunities.begin(), opportunities.end(),
              [](const Opportunity &item1, Opportunity &item2) {
                return item1.pnlVolume.IsNotNan() && item2.pnlVolume.IsNotNan()
                           ? item1.pnlVolume > item2.pnlVolume
                                 ? true
                                 : item1.pnlVolume < item2.pnlVolume
                                       ? false
                                       : item1.pnlRatio > item2.pnlRatio
                           : item1.pnlVolume.IsNotNan();
              });

    if (configurationError) {
      m_opportunitySignal(opportunities);
      throw Exception(configurationError->c_str());
    } else if (!m_isTradingEnabled) {
      m_opportunitySignal(opportunities);
      return;
    }

    auto opportunitySignalFuture = boost::async([this, &opportunities]() {
      try {
        m_opportunitySignal(opportunities);
      } catch (const std::exception &ex) {
        throw boost::enable_current_exception(ex);
      }
    });

    Trade(opportunities, delayMeasurement);

    opportunitySignalFuture.get();
  }

  boost::optional<std::string> CheckSignal(
      std::vector<Opportunity> &opportunities,
      Opportunity::Targets &&targetsSource) {
    opportunities.emplace_back(
        Opportunity{std::move(targetsSource), numberOfLegs,
                    std::numeric_limits<double>::quiet_NaN(),
                    std::numeric_limits<double>::quiet_NaN()});
    auto &opportunity = opportunities.back();
    auto &targets = opportunity.targets;
    boost::array<Double, numberOfCalcQtysPpoints> calcQtysPoints;
    CalcQtys(opportunity, calcQtysPoints);
    opportunity.pnlRatio = m_calcPnlRatio(opportunity.targets);
    if (targets[LEG_3].qty.IsNotNan()) {
      opportunity.pnlVolume =
          m_legs[LEG_3]->CalcPnl(targets[LEG_3].qty, targets[LEG_1].qty);
    }
    if (!opportunity.checkError) {
      boost::tie(opportunity.checkError, opportunity.errorTradingSystem) =
          CheckTargets(opportunity.targets);
    }
    {
      const auto configurationError = CheckCalcs(opportunity);
      if (configurationError) {
#if 0
        if (m_isTradingEnabled) {
          ReportSignal("config. error", opportunity, false);
          return configurationError;
        } else
#else
        ReportSignal("config. error", opportunity, false);
        {
          std::vector<std::string> points;
          for (size_t i = 0; i < points.size(); ++i) {
            if (calcQtysPoints[i].IsNan()) {
              continue;
            }
            boost::format str("%1% = %2%");
            str % i % calcQtysPoints[i];
            points.emplace_back(str.str());
          }
          m_self.GetLog().Error("Configuration error. Points: %1%.",
                                boost::join(points, ", "));
        }
#endif
        {
          if (!opportunity.checkError) {
            Assert(!opportunity.errorTradingSystem);
            static const std::string error("config. error");
            opportunity.checkError = &error;
          }
          return boost::none;
        }
      }
    }
    opportunity.isSignaled = opportunity.pnlVolume.IsNotNan() &&
                             !opportunity.checkError &&
                             opportunity.pnlRatio >= m_minProfitRatio;
    return boost::none;
  }

  void RecheckSignal() {
    m_self.Schedule([this]() { CheckSignal(Milestones()); });
  }

  void Trade(const std::vector<Opportunity> &opportunities,
             const Milestones &delayMeasurement) {
    for (const auto &opportunity : opportunities) {
      if (opportunity.isSignaled) {
        Trade(opportunity, delayMeasurement);
      }
    }
  }

  void Trade(const Opportunity &opportunity,
             const Milestones &delayMeasurement) {
    Assert(opportunity.isSignaled);
    Assert(!opportunity.checkError);
    Assert(!opportunity.errorTradingSystem);

    for (const auto &position : m_self.GetPositions()) {
      if (!position.HasActiveOpenOrders()) {
        return;
      }
      for (const auto &target : opportunity.targets) {
        if (&position.GetTradingSystem() == target.tradingSystem) {
          return;
        }
      }
    }

    boost::optional<Leg> blockedTarget;
    for (size_t i = 0; i < numberOfLegs; ++i) {
      if (!m_failedTargets.count(opportunity.targets[i].tradingSystem)) {
        continue;
      }
      if (blockedTarget) {
        return;
      }
      blockedTarget = static_cast<Leg>(i);
    }

    ReportSignal("trade", opportunity, blockedTarget ? false : true);

    const auto operation =
        boost::make_shared<Operation>(m_self, opportunity.targets);
    OpenPositions(opportunity, operation, blockedTarget, delayMeasurement);
  }

  void OpenPositions(const Opportunity &opportunity,
                     const boost::shared_ptr<Operation> &operation,
                     const boost::optional<Leg> &firstSyncLeg,
                     const Milestones &delayMeasurement) {
    boost::array<Position *, numberOfLegs> positions = {};
    const auto &openPosition = [&](const Leg &leg) {
      Assert(!positions[leg]);
      positions[leg] = m_controller.OpenPosition(
          operation, leg + 1, *opportunity.targets[leg].security,
          delayMeasurement);
    };

    if (firstSyncLeg) {
      try {
        openPosition(*firstSyncLeg);
      } catch (const CommunicationError &ex) {
        ReportSignalError(opportunity, false, *firstSyncLeg, ex.what());
        return;
      }
      m_failedTargets.erase(opportunity.targets[*firstSyncLeg].tradingSystem);
    }

    {
      boost::array<std::unique_ptr<PositionListTransaction::Data>, numberOfLegs>
          positionListTransactions;
      const auto &openPositionAsync = [&](const Leg &leg) {
        try {
          const auto positionListTransaction =
              m_self.StartThreadPositionsTransaction();
          openPosition(leg);
          Assert(!positionListTransactions[leg]);
          positionListTransactions[leg] =
              positionListTransaction->MoveToThread();
        } catch (const CommunicationError &ex) {
          throw boost::enable_current_exception(ex);
        } catch (const Exception &ex) {
          throw boost::enable_current_exception(ex);
        } catch (const std::exception &ex) {
          throw boost::enable_current_exception(ex);
        }
      };

      if (!firstSyncLeg) {
        auto secondLegFuture = boost::async(
            [&openPositionAsync]() { return openPositionAsync(LEG_2); });
        auto thirdLegFuture = boost::async(
            [&openPositionAsync]() { return openPositionAsync(LEG_3); });
        try {
          openPosition(LEG_1);
        } catch (const CommunicationError &ex) {
          ReportSignalError(opportunity, true, LEG_1, ex.what());
        }
        try {
          secondLegFuture.get();
        } catch (const CommunicationError &ex) {
          ReportSignalError(opportunity, true, LEG_2, ex.what());
        }
        try {
          thirdLegFuture.get();
        } catch (const CommunicationError &ex) {
          ReportSignalError(opportunity, true, LEG_3, ex.what());
        }
      } else {
        const auto &asyncLeg = *firstSyncLeg == LEG_2 ? LEG_3 : LEG_2;
        auto future = boost::async([&openPositionAsync, &asyncLeg]() {
          return openPositionAsync(asyncLeg);
        });
        const auto &syncLeg = *firstSyncLeg == LEG_1 ? LEG_3 : LEG_1;
        try {
          openPosition(syncLeg);
        } catch (const CommunicationError &ex) {
          ReportSignalError(opportunity, false, syncLeg, ex.what());
        }
        try {
          future.get();
        } catch (const CommunicationError &ex) {
          ReportSignalError(opportunity, false, asyncLeg, ex.what());
        }
      }
    }

    bool hasErrors = false;
    for (size_t i = 0; i < positions.size(); ++i) {
      if (!positions[i]) {
        Verify(m_failedTargets.emplace(opportunity.targets[i].tradingSystem)
                   .second);
        hasErrors = true;
      }
    }

    if (hasErrors) {
      CloseLegPositionsByOperationStartError(positions);
    }
  }

  void CloseLegPositionsByOperationStartError(
      const boost::array<Position *, numberOfLegs> &positions) {
    Position *position1 = nullptr;
    Position *position2 = nullptr;
    for (auto *position : positions) {
      if (!position) {
        continue;
      }
      Assert(!position1 || !position2);
      (!position1 ? position1 : position2) = position;
    }
    Assert(position1 || position2);

    const auto &close = [this](Position &position) {
      try {
        m_controller.ClosePosition(position, CLOSE_REASON_OPEN_FAILED);
      } catch (const CommunicationError &ex) {
        m_self.GetLog().Warn(
            "Communication error at position closing request: \"%1%\".",
            ex.what());
      }
    };

    if (position1 && position2) {
      const auto positionsTransaction =
          m_self.StartThreadPositionsTransaction();
      auto future = boost::async([&]() { close(*position1); });
      close(*position2);
      future.get();
    } else {
      Assert(position1);
      close(*position1);
    }
  }

  void ReportSignal(const char *signal,
                    const Opportunity &opportunity,
                    bool isAsync) const {
    m_self.GetTradingLog().Write(
        "{'signal': {'%13%': {'pnlRatio': %14%, 'pnlVolume': %15%, 'async': "
        "%16%, 'legs': {'%1%': {'qty': %2%, 'price': %3%, 'side': '%4%'}, "
        "'%5%': {'qty': %6%, 'price': %7%, 'side': '%8%'}, '%9%': {'qty': "
        "%10%, 'price': %11%, 'side': '%12%'}}}}}",
        [&](TradingRecord &record) {
          {
            size_t leg = 0;
            for (const auto &target : opportunity.targets) {
              record % *target.security      // 1, 5, 9
                  % target.qty               // 2, 6, 10
                  % target.price             // 3, 7, 11
                  % m_legs[leg]->GetSide();  // 4, 8, 12
              ++leg;
            }
          }
          record % signal                      // 13
              % opportunity.pnlRatio           // 14
              % opportunity.pnlVolume          // 15
              % (isAsync ? "true" : "false");  // 16
        });
  }

  void ReportSignalError(const Opportunity &opportunity,
                         bool isAsync,
                         const Leg &leg,
                         const char *exception) {
    ReportSignal("error", opportunity, isAsync);
    m_self.GetLog().Warn("Failed to start trading (%1% leg %2%): \"%3%\".",
                         isAsync ? "async" : "sync",  // 1
                         leg + 1,                     // 2
                         exception);                  // 3
  }

  void CalcQtys(Opportunity &opportunity,
                boost::array<Double, numberOfCalcQtysPpoints> &points) const {
    AssertEq(numberOfLegs, opportunity.reducedByAccountBalanceLeg);
    Assert(!opportunity.checkError);
    Assert(!opportunity.errorTradingSystem);
    auto &targets = opportunity.targets;
    points.fill(std::numeric_limits<double>::quiet_NaN());

    const auto disable = [&opportunity](
                             const std::string &error,
                             const TradingSystem &errorTradingSystem) {
      Assert(!opportunity.checkError);
      Assert(!opportunity.errorTradingSystem);
      opportunity.checkError = &error;
      opportunity.errorTradingSystem = &errorTradingSystem;
      for (auto &target : opportunity.targets) {
        target.qty = std::numeric_limits<double>::quiet_NaN();
      }
    };

    if (targets[LEG_1].price == 0 || targets[LEG_2].price == 0) {
      static const std::string error = "failed to get price";
      disable(error, targets[LEG_1].price == 0 ? *targets[LEG_1].tradingSystem
                                               : *targets[LEG_2].tradingSystem);
      AssertLt(0, targets[LEG_1].price);
      AssertLt(0, targets[LEG_2].price);
      return;
    }
    AssertLt(0, targets[LEG_3].price);

    const auto &getBalance = [&](const Leg &leg) {
      return m_legs[leg]->GetOrderQtyAllowedByBalance(
          *targets[leg].tradingSystem, *targets[leg].security,
          targets[leg].price);
    };
    const auto &getMarketQty = [&](const Leg &leg) {
      return m_legs[leg]->GetQty(*targets[leg].security);
    };

    targets[LEG_1].qty =
        std::min(m_maxVolume / targets[LEG_1].price, getMarketQty(LEG_1));
    points[CALC_QTYS_POINT_LEG_1_ORIGINAL] = targets[LEG_1].qty;
    const auto minLeg1Qty = m_minVolume / targets[LEG_1].price;
    const bool isLeg1QtyForced = targets[LEG_1].qty <= minLeg1Qty;
    if (isLeg1QtyForced) {
      targets[LEG_1].qty = minLeg1Qty;
      points[CALC_QTYS_POINT_LEG_1_FORCED] = targets[LEG_1].qty;
    }
    {
      const auto &allowedLeg1Qty = getBalance(LEG_1);
      if (allowedLeg1Qty < targets[LEG_1].qty) {
        targets[LEG_1].qty = allowedLeg1Qty;
        points[CALC_QTYS_POINT_LEG_1_BY_BALANCE] = targets[LEG_1].qty;
        opportunity.reducedByAccountBalanceLeg = LEG_1;
      }
    }
    static const std::string insufficientFundsError = "insufficient funds";
    if (targets[LEG_1].qty == 0) {
      disable(insufficientFundsError, *targets[LEG_1].tradingSystem);
      return;
    }
    const auto leg1SourceVolume = targets[LEG_1].qty * targets[LEG_1].price;

    const auto &reduceQty = [&](const Leg &leg) {
      const auto &balance = getBalance(leg);
      if (!isLeg1QtyForced) {
        const auto &qty = getMarketQty(leg);
        const auto &balanceOrQty = std::min(qty, balance);
        if (balanceOrQty < targets[leg].qty) {
          if (balance < qty) {
            opportunity.reducedByAccountBalanceLeg = leg;
          }
          if (balanceOrQty == 0) {
            disable(insufficientFundsError, *targets[leg].tradingSystem);
            return false;
          }
          targets[leg].qty = balanceOrQty;
        }
      } else if (balance < targets[leg].qty) {
        opportunity.reducedByAccountBalanceLeg = leg;
        if (leg == 0) {
          disable(insufficientFundsError, *targets[leg].tradingSystem);
          return false;
        }
        targets[leg].qty = balance;
      }
      return true;
    };

    targets[LEG_2].qty = leg1SourceVolume;
    points[CALC_QTYS_POINT_LEG_2_ORIGINAL] = targets[LEG_2].qty;
    if (!reduceQty(LEG_2)) {
      return;
    }
    const auto leg2SourceVolume = targets[LEG_2].qty * targets[LEG_2].price;

    targets[LEG_3].qty = leg2SourceVolume / targets[LEG_3].price;
    points[CALC_QTYS_POINT_LEG_3_ORIGINAL] = targets[LEG_3].qty;
    if (!reduceQty(LEG_3)) {
      return;
    }

    {
      const auto &leg3Volume = targets[LEG_3].qty * targets[LEG_3].price;
      if (leg3Volume < leg2SourceVolume) {
        targets[LEG_2].qty = leg3Volume / targets[LEG_2].price;
        points[CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3_SOURCE_VOLUME] =
            leg3Volume;
        points[CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3_SOURCE_PRICE] =
            targets[LEG_2].price;
        points[CALC_QTYS_POINT_LEG_2_REDUCED_BY_LEG_3] = targets[LEG_3].qty;
      }
    }

    if (targets[LEG_2].qty < leg1SourceVolume) {
      targets[LEG_1].qty = targets[LEG_2].qty / targets[LEG_1].price;
      points[CALC_QTYS_POINT_LEG_1_REDUCED_BY_LEG_2_SOURCE_PRICE] =
          targets[LEG_1].price;
      points[CALC_QTYS_POINT_LEG_1_REDUCED_BY_LEG_2] = targets[LEG_1].qty;
    }
  }

  std::pair<const std::string *, const TradingSystem *> CheckTargets(
      Opportunity::Targets &targets) {
    size_t leg = 0;
    for (auto target : targets) {
      const auto *const result =
          OrderBestSecurityChecker::Create(
              m_self, m_legs[leg]->GetSide() == ORDER_SIDE_BUY, target.qty,
              target.price)
              ->Check(*target.security);
      if (result) {
        return {result, target.tradingSystem};
      }
      ++leg;
    }
    return {nullptr, nullptr};
  }

  void SetExchanges(boost::unordered_set<size_t> &&source,
                    boost::unordered_set<size_t> &destination,
                    const char *type) {
    const auto lock = m_self.LockForOtherThreads();
    if (source == destination) {
      return;
    }

    m_self.GetTradingLog().Write(
        "%1% exchange: %2%", [&](TradingRecord &record) {
          record % type;  // 1
          if (source.empty()) {
            record % "any exchange";  // 2
          } else {
            std::vector<std::string> names;
            names.reserve(source.size());
            for (const auto &index : source) {
              names.emplace_back(m_self.GetContext()
                                     .GetMarketDataSource(index)
                                     .GetInstanceName());
            }
            record % boost::join(names, ", ");  // 2
          }
        });
    destination = std::move(source);
    RecheckSignal();
  }

  bool IsExchangeEnabled(const Security &leg,
                         const boost::unordered_set<size_t> &set) const {
    return set.empty() || set.count(leg.GetSource().GetIndex()) > 0;
  }
};

////////////////////////////////////////////////////////////////////////////////

ta::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf)
    : Base(context,
           "{F0F45162-F1D3-484A-A0F3-7AC7DF7F9DA9}",
           "TriangularArbitrage",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {
  {
    m_pimpl->m_isTradingEnabled = conf.ReadBoolKey("is_trading_enabled");
    m_pimpl->m_minVolume = conf.ReadTypedKey<Volume>("min_volume");
    m_pimpl->m_maxVolume = conf.ReadTypedKey<Volume>("max_volume");
    m_pimpl->m_minProfitRatio = conf.ReadTypedKey<Double>("min_profit_ratio");
  }
  {
    std::vector<std::pair<std::string, OrderSide>> legsConf;
    legsConf.reserve(numberOfLegs);
    size_t numberOfShorts = 0;
    size_t numberOfLongs = 0;
    for (const auto &legConf : conf.ReadList("legs", ",", true)) {
      if (legConf.size() < 2 || (legConf[0] != '-' && legConf[0] != '+')) {
        throw Exception("Wrong leg configuration in leg set configuration");
      }
      legsConf.emplace_back(legConf.substr(1), legConf[0] == '+'
                                                   ? ORDER_SIDE_LONG
                                                   : ORDER_SIDE_SHORT);
      ++(legsConf.back().second == ORDER_SIDE_LONG ? numberOfLongs
                                                   : numberOfShorts);
    }
    auto legIt = m_pimpl->m_legs.begin();
    for (const auto &legConf : legsConf) {
      if (legConf.second == ORDER_SIDE_LONG) {
        *legIt =
            boost::make_unique<OppositeLegPolicy<BuyLegPolicy>>(legConf.first);
      } else {
        *legIt =
            boost::make_unique<DirectLegPolicy<SellLegPolicy>>(legConf.first);
      }
      ++legIt;
    }
    if (legIt != m_pimpl->m_legs.cend()) {
      throw Exception("Too few legs configured");
    }
    AssertEq(numberOfLegs, numberOfLongs + numberOfShorts);
    AssertLe(1, numberOfLongs);
    AssertLe(1, numberOfShorts);
    if (numberOfLongs < numberOfShorts) {
      m_pimpl->m_calcPnlRatio = [this](const Opportunity::Targets &targets) {
        const auto &calcX = [this, &targets](const Leg &leg) {
          return m_pimpl->m_legs[leg]->CalcX(*targets[leg].security);
        };
        return calcX(LEG_1) * calcX(LEG_2) * calcX(LEG_3);
      };
    } else {
      m_pimpl->m_calcPnlRatio = [this](const Opportunity::Targets &targets) {
        const auto &calcX = [this, &targets](const Leg &leg) {
          return m_pimpl->m_legs[leg]->CalcX(*targets[leg].security);
        };
        return calcX(LEG_3) * calcX(LEG_2) * calcX(LEG_1);
      };
    }
  }
  GetLog().Info("Is trading enabled: %1%.",
                m_pimpl->m_isTradingEnabled ? "yes" : "no");
  GetLog().Info("Volume: %1% - %2%.",
                m_pimpl->m_minVolume,   // 1
                m_pimpl->m_maxVolume);  // 2
  GetLog().Info("Min. profit ratio: %1%.", m_pimpl->m_minProfitRatio);
  {
    std::vector<std::string> legs;
    for (const auto &leg : m_pimpl->m_legs) {
      boost::format legStr("%1%(%2%)");
      legStr % leg->GetSymbol() % leg->GetSide();
      legs.emplace_back(legStr.str());
    }
    GetLog().Info("Legs: %1%.", boost::join(legs, ", "));
  }
}

ta::Strategy::~Strategy() = default;

void ta::Strategy::Stop() noexcept {
  const auto lock = LockForOtherThreads();
  GetTradingLog().Write("stopped");
  m_pimpl->m_isStopped = true;
}

sig::scoped_connection ta::Strategy::SubscribeToOpportunity(
    const boost::function<void(const std::vector<Opportunity> &)> &slot) {
  return m_pimpl->m_opportunitySignal.connect(slot);
}

sig::scoped_connection ta::Strategy::SubscribeToBlocking(
    const boost::function<void(const std::string *reason)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

void ta::Strategy::OnSecurityStart(Security &security,
                                   Security::Request &request) {
  if (m_pimpl->m_isStopped) {
    return;
  }

  bool isSet = false;
  {
    size_t legNo = 0;
    for (auto &leg : m_pimpl->m_legs) {
      ++legNo;
      if (security.GetSymbol().GetSymbol() != leg->GetSymbol()) {
        continue;
      }
      GetLog().Debug("Set \"%1%\" for leg #%2% \"%3%\" (%4%).",
                     security,          // 1
                     legNo,             // 2
                     leg->GetSymbol(),  // 3
                     leg->GetSide());   // 4
      leg->AddSecurities(security);
      isSet = true;
    }
  }

  if (!isSet) {
    throw Exception("Failed to find configured leg for security");
  }

  {
    size_t numberOfSecuriries = 0;
    for (const auto &leg : m_pimpl->m_legs) {
      numberOfSecuriries += leg->GetSecurities().size();
    }
    m_pimpl->m_numberOfSecuriries = numberOfSecuriries;
  }

  Base::OnSecurityStart(security, request);
}

void ta::Strategy::OnLevel1Update(Security &,
                                  const Milestones &delayMeasurement) {
  if (m_pimpl->m_isStopped) {
    return;
  }
  m_pimpl->CheckSignal(delayMeasurement);
}

void ta::Strategy::OnPositionUpdate(Position &position) {
  if (m_pimpl->m_isStopped) {
    return;
  }
  try {
    m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Warn("Communication error at position update handling: \"%1%\".",
                  ex.what());
    return;
  }
  if (!position.HasActiveOpenOrders()) {
    m_pimpl->RecheckSignal();
  }
}

void ta::Strategy::OnPostionsCloseRequest() {
  if (m_pimpl->m_isStopped) {
    return;
  }
  m_pimpl->m_controller.OnPostionsCloseRequest(*this);
}

bool ta::Strategy::OnBlocked(const std::string *reason) noexcept {
  if (m_pimpl->m_isStopped) {
    return Base::OnBlocked(reason);
  }
  try {
    m_pimpl->m_blockSignal(reason);
  } catch (...) {
    AssertFailNoException();
    return Base::OnBlocked(reason);
  }
  return false;
}

bool ta::Strategy::IsTradingEnabled() const {
  return m_pimpl->m_isTradingEnabled;
}
void ta::Strategy::EnableTrading(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_isTradingEnabled == isEnabled) {
    return;
  }
  GetTradingLog().Write("trading: %1% -> %2%", [&](TradingRecord &record) {
    record % (m_pimpl->m_isTradingEnabled ? "enabled" : "disabled")  // 1
        % (isEnabled ? "enabled" : "disabled");                      // 2
  });
  m_pimpl->m_isTradingEnabled = isEnabled;
  m_pimpl->RecheckSignal();
}

const Volume &ta::Strategy::GetMinVolume() const {
  return m_pimpl->m_minVolume;
}
void ta::Strategy::SetMinVolume(const Volume &minVolume) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_minVolume == minVolume) {
    return;
  }
  const auto maxVolume = std::max(m_pimpl->m_maxVolume, minVolume);
  GetTradingLog().Write("volume: %1%-%2% -> %3%-%4%",
                        [&](TradingRecord &record) {
                          record % m_pimpl->m_minVolume  // 1
                              % m_pimpl->m_maxVolume     // 2
                              % minVolume                // 3
                              % maxVolume;               // 4
                        });
  m_pimpl->m_minVolume = minVolume;
  m_pimpl->m_maxVolume = maxVolume;
  m_pimpl->RecheckSignal();
}
const Volume &ta::Strategy::GetMaxVolume() const {
  return m_pimpl->m_maxVolume;
}
void ta::Strategy::SetMaxVolume(const Volume &maxVolume) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_maxVolume == maxVolume) {
    return;
  }
  const auto minVolume = std::min(m_pimpl->m_minVolume, maxVolume);
  GetTradingLog().Write("volume: %1%-%2% -> %3%-%4%",
                        [&](TradingRecord &record) {
                          record % m_pimpl->m_minVolume  // 1
                              % m_pimpl->m_maxVolume     // 2
                              % minVolume                // 3
                              % maxVolume;               // 4
                        });
  m_pimpl->m_minVolume = minVolume;
  m_pimpl->m_maxVolume = maxVolume;
  m_pimpl->RecheckSignal();
}

const Double &ta::Strategy::GetMinProfitRatio() const {
  return m_pimpl->m_minProfitRatio;
}
void ta::Strategy::SetMinProfitRatio(const Double &minProfitRatio) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_minProfitRatio == minProfitRatio) {
    return;
  }
  GetTradingLog().Write("min. profit ratio: %1% -> %2%",
                        [&](TradingRecord &record) {
                          record % m_pimpl->m_minProfitRatio  // 1
                              % minProfitRatio;               // 2
                        });
  m_pimpl->m_minProfitRatio = minProfitRatio;
  m_pimpl->RecheckSignal();
}

const boost::unordered_set<size_t> &ta::Strategy::GetStartExchanges() const {
  return m_pimpl->m_startExchanges;
}
void ta::Strategy::SetStartExchanges(boost::unordered_set<size_t> &&exchanges) {
  m_pimpl->SetExchanges(std::move(exchanges), m_pimpl->m_startExchanges,
                        "start");
}
const boost::unordered_set<size_t> &ta::Strategy::GetMiddleExchanges() const {
  return m_pimpl->m_middleExchanges;
}
void ta::Strategy::SetMiddleExchanges(
    boost::unordered_set<size_t> &&exchanges) {
  m_pimpl->SetExchanges(std::move(exchanges), m_pimpl->m_middleExchanges,
                        "middle");
}
const boost::unordered_set<size_t> &ta::Strategy::GetFinishExchanges() const {
  return m_pimpl->m_finishExchanges;
}
void ta::Strategy::SetFinishExchanges(
    boost::unordered_set<size_t> &&exchanges) {
  m_pimpl->SetExchanges(std::move(exchanges), m_pimpl->m_finishExchanges,
                        "finish");
}

const boost::array<std::unique_ptr<LegPolicy>, numberOfLegs>
    &ta::Strategy::GetLegs() const {
  return m_pimpl->m_legs;
}

Leg ta::Strategy::GetLeg(const Security &security) const {
  for (size_t i = 0; m_pimpl->m_legs.size(); ++i) {
    if (m_pimpl->m_legs[i]->GetSecurities().count(
            const_cast<Security *>(&security)) > 0) {
      return static_cast<Leg>(i);
    }
  }
  throw Exception("Failed to find leg by security");
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf) {
  return boost::make_unique<ta::Strategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
