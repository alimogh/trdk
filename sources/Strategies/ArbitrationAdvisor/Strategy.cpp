/*******************************************************************************
 *   Created: 2017/10/22 17:57:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "Operation.hpp"
#include "PositionController.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;
using namespace Strategies::ArbitrageAdvisor;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace sig = boost::signals2;
namespace ids = boost::uuids;
namespace aa = Strategies::ArbitrageAdvisor;

////////////////////////////////////////////////////////////////////////////////

namespace {

typedef std::pair<Price, AdviceSecuritySignal *> PriceItem;

Price CaclSpread(const Price &bid, const Price &ask) { return bid - ask; }
std::pair<Price, Double> CaclSpreadAndRatio(const Price &bid,
                                            const Price &ask) {
  const auto spread = CaclSpread(bid, ask);
  auto ratio = spread != 0 ? (100 / (ask / spread)) : 0;
  ratio = RoundByPrecisionPower(ratio, 100);
  ratio /= 100;
  return {spread, ratio};
}
std::pair<Price, Double> CaclSpreadAndRatio(const PriceItem &bestBid,
                                            const PriceItem &bestAsk) {
  return CaclSpreadAndRatio(bestBid.first, bestAsk.first);
}
std::pair<Price, Double> CaclSpreadAndRatio(const Security &sellTarget,
                                            const Security &buyTarget) {
  return CaclSpreadAndRatio(sellTarget.GetBidPriceValue(),
                            buyTarget.GetAskPriceValue());
}

class SignalSession : private boost::noncopyable {
 public:
  Qty TakeQty(const Security &target, bool isSell, const Qty &originalQty) {
    auto &storage = !isSell ? m_usedQtyToBuy : m_usedQtyToSell;
    const auto &it = storage.find(&target);
    if (it == storage.cend()) {
      Verify(storage.emplace(&target, originalQty).second);
      return originalQty;
    }
    const auto result = originalQty > it->second ? originalQty - it->second : 0;
    it->second += result;
    return result;
  }

  void ReturnQty(const Security &target,
                 bool isSell,
                 const Qty &takenQty,
                 const Qty &usedQty) {
    AssertGe(takenQty, usedQty);
    auto &storage = !isSell ? m_usedQtyToBuy : m_usedQtyToSell;
    const auto &it = storage.find(&target);
    Assert(it != storage.cend());
    if (it != storage.cend()) {
      return;
    }
    AssertGe(it->second, takenQty - usedQty);
    it->second -= takenQty - usedQty;
  }

  void RegisterCheckError(
      const Security &sellTarget,
      const Security &buyTarget,
      const Security &checkTarget,
      const std::string &error,
      const boost::optional<const OrderCheckError> &orderCheckError) {
    Verify(
        m_checkErrors
            .emplace(std::make_pair(&sellTarget, &buyTarget),
                     boost::make_tuple(&error, &checkTarget, orderCheckError))
            .second);
  }
  void RegisterCheckError(const Security &sellTarget,
                          const Security &buyTarget,
                          const std::string &error) {
    Verify(m_checkErrors
               .emplace(std::make_pair(&sellTarget, &buyTarget),
                        boost::make_tuple(&error, nullptr, boost::none))
               .second);
  }
  void ForEachCheckError(
      const boost::function<void(const Security &sellTarget,
                                 const Security &buyTarget,
                                 const std::string &error,
                                 const Security *checkTarget,
                                 const boost::optional<const OrderCheckError>
                                     &orderCheckError)> &callback) const {
    for (const auto &targetSet : m_checkErrors) {
      callback(*targetSet.first.first, *targetSet.first.second,
               *boost::get<0>(targetSet.second),
               boost::get<1>(targetSet.second),
               boost::get<2>(targetSet.second));
    }
  }

 private:
  boost::unordered_map<const Security *, Qty> m_usedQtyToBuy;
  boost::unordered_map<const Security *, Qty> m_usedQtyToSell;

  std::map<std::pair<const Security *, const Security *>,
           boost::tuple<const std::string *,
                        const Security *,
                        boost::optional<const OrderCheckError>>>
      m_checkErrors;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

const ids::uuid aa::Strategy::typeId =
    ids::string_generator()("{39FBFFDA-10D7-462D-BA82-0D8BA9CA7A09}");

class aa::Strategy::Implementation : private boost::noncopyable {
 public:
  aa::Strategy &m_self;

  const bool m_isLowestSpreadEnabled;
  const Double m_lowestSpreadRatio;

  const bool m_isStopLossEnabled;
  const pt::time_duration m_stopLossDelay;

  PositionController m_controller;

  sig::signal<void(const Advice &)> m_adviceSignal;
  sig::signal<void(const std::vector<std::string> &)>
      m_tradingSignalCheckErrorsSignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  Double m_minPriceDifferenceRatioToAdvice;
  TradingSettings m_tradingSettings;

  boost::unordered_map<Symbol, std::vector<AdviceSecuritySignal>> m_symbols;

  boost::unordered_set<const Security *> m_errors;

  mutable std::vector<std::string> m_lastTradingSignalCheckErrors;

  explicit Implementation(aa::Strategy &self, const ptr::ptree &conf)
      : m_self(self),
        m_isLowestSpreadEnabled(conf.get<bool>("config.isLowestSpreadEnabled")),
        m_lowestSpreadRatio(conf.get<Double>("config.lowestSpreadPercentage") /
                            100),
        m_isStopLossEnabled(conf.get<bool>("config.isStopLossEnabled")),
        m_stopLossDelay(pt::seconds(conf.get<long>("config.stopLossDelaySec"))),
        m_minPriceDifferenceRatioToAdvice(
            conf.get<Double>("config.minPriceDifferenceToHighlightPercentage") /
            100),
        m_tradingSettings(
            {conf.get<bool>("config.isAutoTradingEnabled"),
             conf.get<Double>("config.minPriceDifferenceToTradePercentage") /
                 100,
             conf.get<Qty>("config.maxQty")}) {
    m_self.GetLog().Info(
        "Min. price diff. to highlight: %1%%%. Trading: %2%. Min. price diff. "
        "to trade: %3%%%. Max. qty.: %4%.",
        m_minPriceDifferenceRatioToAdvice * 100,               // 1
        m_tradingSettings.isEnabled ? "enabled" : "disabled",  // 2
        m_tradingSettings.minPriceDifferenceRatio * 100,       // 3
        m_tradingSettings.maxQty);                             // 4

    if (m_isLowestSpreadEnabled) {
      m_self.GetLog().Info("Lowest spread: %1%%%.",
                           m_isLowestSpreadEnabled * 100);
    }
    if (m_isStopLossEnabled) {
      m_self.GetLog().Info("Stop-loss uses delay %1%.", m_stopLossDelay);
    }
  }

  void CheckAutoTradingSignal(Security &security,
                              const Milestones &delayMeasurement) {
    if (m_adviceSignal.num_slots() == 0 && !m_tradingSettings.isEnabled) {
      return;
    }
    AssertLt(0, m_adviceSignal.num_slots());
    CheckSignal(security, m_symbols[security.GetSymbol()], delayMeasurement);
  }

  void CheckSignal(Security &updatedSecurity,
                   std::vector<AdviceSecuritySignal> &allSecurities,
                   const Milestones &delayMeasurement) {
    CheckTradeSignal(allSecurities, delayMeasurement);
    CheckAdviceSignal(updatedSecurity, allSecurities);
  }

  void CheckAdviceSignal(Security &updatedSecurity,
                         std::vector<AdviceSecuritySignal> &allSecurities) {
    std::vector<PriceItem> bids;
    std::vector<PriceItem> asks;

    bids.reserve(allSecurities.size());
    asks.reserve(allSecurities.size());

    Price updatedSecurityBidPrice = std::numeric_limits<double>::quiet_NaN();
    Price updatedSecurityAskPrice = std::numeric_limits<double>::quiet_NaN();
    for (auto &security : allSecurities) {
      security.isBestBid = security.isBestAsk = false;
      bids.emplace_back(security.security->GetBidPriceValue(), &security);
      asks.emplace_back(security.security->GetAskPriceValue(), &security);
      if (security.security == &updatedSecurity) {
        updatedSecurityBidPrice = bids.back().first;
        updatedSecurityAskPrice = asks.back().first;
      }
      if (bids.back().first.IsNan()) {
        bids.pop_back();
      }
      if (asks.back().first.IsNan()) {
        asks.pop_back();
      }
    }

    std::sort(bids.begin(), bids.end(),
              [](const PriceItem &lhs, const PriceItem &rhs) {
                return lhs.first > rhs.first;
              });
    std::sort(asks.begin(), asks.end(),
              [](const PriceItem &lhs, const PriceItem &rhs) {
                return lhs.first < rhs.first;
              });

    Price spread;
    Double spreadRatio;
    if (!bids.empty() && !asks.empty()) {
      const auto &bestBid = bids.front();
      const auto &bestAsk = asks.front();
      boost::tie(spread, spreadRatio) = CaclSpreadAndRatio(bestBid, bestAsk);
    } else {
      spread = spreadRatio = std::numeric_limits<double>::quiet_NaN();
    }

    for (auto it = bids.begin();
         it != bids.cend() && it->first == bids.front().first; ++it) {
      Assert(!it->second->isBestBid);
      it->second->isBestBid = true;
    }
    for (auto it = asks.begin();
         it != asks.cend() && it->first == asks.front().first; ++it) {
      Assert(!it->second->isBestAsk);
      it->second->isBestAsk = true;
    }

    m_adviceSignal(
        Advice{&updatedSecurity,
               updatedSecurity.GetLastMarketDataTime(),
               {updatedSecurityBidPrice, updatedSecurity.GetBidQtyValue()},
               {updatedSecurityAskPrice, updatedSecurity.GetAskQtyValue()},
               spread,
               spreadRatio,
               spreadRatio >= m_minPriceDifferenceRatioToAdvice,
               allSecurities});
  }

  void CheckTradeSignal(std::vector<AdviceSecuritySignal> &allSecurities,
                        const Milestones &delayMeasurement) {
    struct SignalData {
      Double spreadRatio;
      Security *sellTarget;
      Security *buyTarget;

      explicit SignalData(Security &sellTarget, Security &buyTarget)
          : spreadRatio(CaclSpreadAndRatio(sellTarget, buyTarget).second),
            sellTarget(&sellTarget),
            buyTarget(&buyTarget) {
        Assert(this->sellTarget != this->buyTarget);
      }
    };
    std::vector<SignalData> signalSet;
    signalSet.reserve(allSecurities.size() * allSecurities.size());

    for (auto &sellTarget : allSecurities) {
      if (!sellTarget.security->IsOnline()) {
        continue;
      }
      for (auto &buyTarget : allSecurities) {
        if (&sellTarget == &buyTarget || !buyTarget.security->IsOnline()) {
          continue;
        }
        Assert(sellTarget.security != buyTarget.security);
        signalSet.emplace_back(*sellTarget.security, *buyTarget.security);
      }
    }

    std::sort(signalSet.begin(), signalSet.end(),
              [](const SignalData &lhs, const SignalData &rhs) {
                return std::fabsl(lhs.spreadRatio) >
                       std::fabsl(rhs.spreadRatio);
              });

    SignalSession session;
    for (const auto &signal : signalSet) {
      auto &sellTarget = *signal.sellTarget;
      auto &buyTarget = *signal.buyTarget;
      const auto &spreadRatio =
          CaclSpreadAndRatio(sellTarget, buyTarget).second;
      if (!m_tradingSettings.isEnabled ||
          spreadRatio < m_tradingSettings.minPriceDifferenceRatio ||
          CheckActualPositions(sellTarget, buyTarget)) {
        continue;
      }
      Trade(sellTarget, buyTarget, spreadRatio, session, delayMeasurement);
    }

    ReportSignalCheckErrors(session);
  }

  void RecheckSignalSync() {
    for (auto &symbol : m_symbols) {
      for (const auto &security : symbol.second) {
        CheckSignal(*security.security, symbol.second, Milestones());
      }
    }
  }

  void RecheckSignalAsync() {
    m_self.Schedule([this]() { RecheckSignalSync(); });
  }

  static bool CheckActualPosition(const Position &position,
                                  const Security &sellTarget,
                                  const Security &buyTarget) {
    return !position.IsCompleted() &&
           position.GetTypedOperation<aa::Operation>().IsSame(sellTarget,
                                                              buyTarget);
  }

  bool CheckActualPositions(const Security &sellTarget,
                            const Security &buyTarget) {
    for (auto &position : m_self.GetPositions()) {
      if (CheckActualPosition(position, sellTarget, buyTarget)) {
        return true;
      }
    }
    return false;
  }

  void Trade(Security &sellTarget,
             Security &buyTarget,
             const Double &bestSpreadRatio,
             SignalSession &signalSession,
             const Milestones &delayMeasurement) {
    auto sellPrice = sellTarget.GetBidPrice();
    auto buyPrice = buyTarget.GetAskPrice();
    const auto spreadRatio = m_isLowestSpreadEnabled
                                 ? m_lowestSpreadRatio
                                 : m_tradingSettings.minPriceDifferenceRatio;
    {
      const auto spreadHalfPercent = (spreadRatio * 100) / 2;
      const auto middle = buyPrice + ((sellPrice - buyPrice) / 2);
      const auto newBuyPrice =
          middle - ((middle * spreadHalfPercent) / (100 + spreadHalfPercent));
      buyPrice = std::move(newBuyPrice);
      const auto newSellPrice = buyPrice * (spreadRatio + 1);
      sellPrice = std::move(newSellPrice);
    }

    struct Qtys : private boost::noncopyable {
      SignalSession *session;
      Security &sellTarget;
      Security &buyTarget;
      Qty sellQty;
      Qty buyQty;

      explicit Qtys(Security &sellTarget,
                    Security &buyTarget,
                    SignalSession &session)
          : session(&session),
            sellTarget(sellTarget),
            buyTarget(buyTarget),
            sellQty(sellTarget.GetBidQty()),
            buyQty(buyTarget.GetAskQty()) {
        sellQty = session.TakeQty(sellTarget, true, sellQty);
        buyQty = session.TakeQty(buyTarget, false, buyQty);
      }
      ~Qtys() { Return(0); }

      void Return(const Qty &used) {
        if (!session) {
          return;
        }
        session->ReturnQty(sellTarget, true, sellQty, used);
        session->ReturnQty(buyTarget, false, buyQty, used);
        session = nullptr;
      }
    } qtys(sellTarget, buyTarget, signalSession);

    auto qty =
        std::min(m_tradingSettings.maxQty, std::min(qtys.sellQty, qtys.buyQty));

    {
      const auto &allowedQty =
          GetOrderQtyAllowedByBalance(sellTarget, buyTarget, buyPrice);
      if (qty > allowedQty && allowedQty > 0) {
        qty = allowedQty;
      }
    }

    {
      const auto &checkTarget = [&](Security &target, bool isBuy,
                                    const Price &price) -> bool {
        const auto &result =
            OrderBestSecurityChecker::Create(m_self, isBuy, qty, price)
                ->Check(target);
        if (!result) {
          return true;
        }
        signalSession.RegisterCheckError(sellTarget, buyTarget, target,
                                         result->GetRuleNameRef(),
                                         result->GetOrderError());
        return false;
      };
      if (!checkTarget(sellTarget, false, sellPrice) ||
          !checkTarget(buyTarget, true, buyPrice)) {
        return;
      }
    }

    const auto sellTargetBlackListIt = m_errors.find(&sellTarget);
    const auto isSellTargetInBlackList =
        sellTargetBlackListIt != m_errors.cend();
    const auto buyTargetBlackListIt = m_errors.find(&buyTarget);
    const auto isBuyTargetInBlackList = buyTargetBlackListIt != m_errors.cend();
    if (isSellTargetInBlackList && isBuyTargetInBlackList) {
      static const std::string error = "both targets on the blocked list";
      signalSession.RegisterCheckError(sellTarget, buyTarget, error);
      return;
    }

    ReportSignal("trade", "start", sellTarget, buyTarget, spreadRatio,
                 bestSpreadRatio);

    boost::optional<pt::time_duration> stopLossDelay;
    if (m_isStopLossEnabled) {
      stopLossDelay = m_stopLossDelay;
    }
    const auto operation = boost::make_shared<Operation>(
        m_self, sellTarget, buyTarget, qty, sellPrice, buyPrice,
        std::move(stopLossDelay));

    qtys.Return(qty);

    if (isSellTargetInBlackList || isBuyTargetInBlackList) {
      if (!OpenPositionSync(sellTarget, buyTarget, operation,
                            isBuyTargetInBlackList, spreadRatio,
                            bestSpreadRatio, delayMeasurement)) {
        return;
      }
    } else if (!OpenPositionAsync(sellTarget, buyTarget, operation, spreadRatio,
                                  bestSpreadRatio, delayMeasurement)) {
      return;
    }

    if (isBuyTargetInBlackList) {
      m_errors.erase(buyTargetBlackListIt);
    }
    if (isSellTargetInBlackList) {
      m_errors.erase(sellTargetBlackListIt);
    }
  }

  Qty GetOrderQtyAllowedByBalance(const Security &sell,
                                  const Security &buy,
                                  const Price &buyPrice) {
    const auto &sellBalance =
        m_self.GetTradingSystem(sell.GetSource().GetIndex())
            .GetBalances()
            .GetAvailableToTrade(sell.GetSymbol().GetBaseSymbol());

    const auto &buyTradingSystem =
        m_self.GetTradingSystem(buy.GetSource().GetIndex());
    auto buyBalance = buyTradingSystem.GetBalances().GetAvailableToTrade(
        buy.GetSymbol().GetQuoteSymbol());
    buyBalance -= buyTradingSystem.CalcCommission(
        buyBalance / buyPrice, buyPrice, ORDER_SIDE_BUY, buy);

    return std::min(sellBalance, buyBalance / buyPrice);
  }

  void ReportSignal(const char *type,
                    const char *reason,
                    const Security &sellTarget,
                    const Security &buyTarget,
                    const Double &spreadRatio,
                    const Double &bestSpreadRatio,
                    const std::string &additional = std::string()) const {
    m_self.GetTradingLog().Write(
        "{'signal': {'%14%': {'reason': '%9%', 'sell': {'security': '%1%', "
        "'bid': {'price': %2%, 'qty': %10%}, 'ask': {'price': %3%, 'qty': "
        "%11%}}, 'buy': {'security': '%4%', 'bid': {'price': %5%, 'qty': "
        "%12%}, 'ask': {'price': %6%, 'qty': %13%}}}, 'spread': {'used': "
        "%7$.3f, 'signal': %8$.3f}%15%}}",
        [&](TradingRecord &record) {
          record % sellTarget                  // 1
              % sellTarget.GetBidPriceValue()  // 2
              % sellTarget.GetAskPriceValue()  // 3
              % buyTarget                      // 4
              % buyTarget.GetBidPriceValue()   // 5
              % buyTarget.GetAskPriceValue()   // 6
              % (spreadRatio * 100)            // 7
              % (bestSpreadRatio * 100)        // 8
              % reason                         // 9
              % sellTarget.GetBidQtyValue()    // 10
              % sellTarget.GetAskQtyValue()    // 11
              % buyTarget.GetBidQtyValue()     // 12
              % buyTarget.GetAskQtyValue()     // 13
              % type                           // 14
              % additional;                    // 15
        });
  }

  void ReportSignalCheckErrors(const SignalSession &session) const {
    std::vector<std::string> report;
    session.ForEachCheckError(
        [this, &report](
            const Security &sellTarget, const Security &buyTarget,
            const std::string &ruleName, const Security *checkTarget,
            const boost::optional<const OrderCheckError> &orderError) {
          std::ostringstream os;
          os << m_self.GetTradingSystem(sellTarget.GetSource().GetIndex())
                    .GetTitle()
             << " > "
             << m_self.GetTradingSystem(buyTarget.GetSource().GetIndex())
                    .GetTitle()
             << ": " << ruleName;
          Assert(checkTarget || !orderError);
          if (checkTarget) {
            os << " ("
               << m_self.GetTradingSystem(checkTarget->GetSource().GetIndex())
                      .GetTitle();
            if (orderError) {
              os << " requires: ";
              auto has = false;
              if (orderError->qty) {
                os << " qty. >= " << *orderError->qty;
                has = true;
              }
              if (orderError->price) {
                if (has) {
                  os << ',';
                }
                os << " price >= " << *orderError->price;
                has = true;
              }
              if (orderError->volume) {
                if (has) {
                  os << ',';
                }
                os << " volume >= " << *orderError->volume;
                has = true;
              }
            }
            os << ")";
          }
          report.emplace_back(os.str());
        });
    if (report == m_lastTradingSignalCheckErrors) {
      return;
    }
    m_lastTradingSignalCheckErrors = std::move(report);
    m_self.GetTradingLog().Write(
        "signal check errors: {%1%}", [this](TradingRecord &record) {
          record % boost::join(m_lastTradingSignalCheckErrors, "}, {");
        });
    m_tradingSignalCheckErrorsSignal(m_lastTradingSignalCheckErrors);
  }

  bool OpenPositionSync(Security &sellTarget,
                        Security &buyTarget,
                        const boost::shared_ptr<Operation> &operation,
                        bool isBuyTargetInBlackList,
                        const Double &spreadRatio,
                        const Double &bestSpreadRatio,
                        const Milestones &delayMeasurement) {
    const auto &legTargets = isBuyTargetInBlackList
                                 ? std::make_pair(&buyTarget, &sellTarget)
                                 : std::make_pair(&sellTarget, &buyTarget);

    Position *firstLeg = nullptr;
    try {
      firstLeg =
          m_controller.Open(operation, 1, *legTargets.first, delayMeasurement);
      if (!firstLeg) {
        return false;
      }
      m_controller.Open(operation, 2, *legTargets.second, delayMeasurement);

      return true;

    } catch (const CommunicationError &ex) {
      ReportSignal("error", "sync", sellTarget, buyTarget, spreadRatio,
                   bestSpreadRatio);
      m_self.GetLog().Warn("Failed to start trading (sync): \"%1%\".",
                           ex.what());

      if (firstLeg) {
        CloseLegPositionByOperationStartError(*firstLeg);
      }

      const auto &errorLeg =
          *(!firstLeg ? legTargets.first : legTargets.second);
      m_errors.emplace(&errorLeg);
      m_self.GetLog().Debug(
          "\"%1%\" (%2% leg) added to the blacklist by position opening "
          "error. "
          "%3% leg is \"%4%\"",
          errorLeg,                        // 1
          !firstLeg ? "first" : "second",  // 2
          !firstLeg ? "Second" : "First",  // 3
          &errorLeg == legTargets.first ? *legTargets.second
                                        : *legTargets.first);  // 4
    }

    return false;
  }

  bool OpenPositionAsync(Security &sellTarget,
                         Security &buyTarget,
                         const boost::shared_ptr<Operation> &operation,
                         const Double &spreadRatio,
                         const Double &bestSpreadRatio,
                         const Milestones &delayMeasurement) {
    const auto &openPosition = [&](int64_t leg,
                                   Security &target) -> Position * {
      return m_controller.Open(operation, leg, target, delayMeasurement);
    };

    auto &firstLegTarget = sellTarget;
    auto &secondLegTarget = buyTarget;

    Position *firstLeg = nullptr;
    Position *secondLeg = nullptr;
    {
      const auto secondLegPositionsTransaction =
          m_self.StartThreadPositionsTransaction();
      auto firstLegFuture =
          boost::async([&openPosition, &firstLegTarget]() -> Position * {
            try {
              return openPosition(1, firstLegTarget);
            } catch (const CommunicationError &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const Exception &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const std::exception &ex) {
              throw boost::enable_current_exception(ex);
            }
          });
      try {
        secondLeg = openPosition(2, secondLegTarget);
      } catch (const CommunicationError &ex) {
        ReportSignal("error", "async 2nd leg", sellTarget, buyTarget,
                     spreadRatio, bestSpreadRatio);
        m_self.GetLog().Warn(
            "Failed to start trading (async 2nd leg): \"%1%\".", ex.what());
      }
      try {
        firstLeg = firstLegFuture.get();
      } catch (const CommunicationError &ex) {
        ReportSignal("error", "async 1st leg", sellTarget, buyTarget,
                     spreadRatio, bestSpreadRatio);
        m_self.GetLog().Warn(
            "Failed to start trading (async 1st leg): \"%1%\".", ex.what());
      }
    }
    if (firstLeg && secondLeg) {
      return true;
    }

    if (!firstLeg) {
      Verify(m_errors.emplace(&firstLegTarget).second);
    }
    if (!secondLeg) {
      Verify(m_errors.emplace(&secondLegTarget).second);
    }

    if (!firstLeg && !secondLeg) {
      m_self.GetLog().Debug(
          "\"%1%\" and \"%2%\" added to the blacklist by position opening "
          "error.",
          firstLegTarget,    // 1
          secondLegTarget);  // 2
      return false;
    }

    const Security *errorLeg;
    if (firstLeg) {
      Assert(!secondLeg);
      errorLeg = &secondLegTarget;
      CloseLegPositionByOperationStartError(*firstLeg);
    } else {
      Assert(secondLeg);
      errorLeg = &firstLegTarget;
      CloseLegPositionByOperationStartError(*secondLeg);
    }
    m_self.GetLog().Debug(
        "\"%1%\" (%2% leg) added to the blacklist by position opening error. "
        "%3% leg is \"%4%\"",
        *errorLeg,                                      // 1
        firstLeg ? "second" : "first",                  // 2
        secondLeg ? "Second" : "First",                 // 3
        secondLeg ? secondLegTarget : firstLegTarget);  // 4

    return false;
  }

  void CloseLegPositionByOperationStartError(Position &openedPosition) {
    try {
      m_controller.Close(openedPosition, CLOSE_REASON_OPEN_FAILED);
    } catch (const CommunicationError &ex) {
      m_self.GetLog().Warn(
          "Communication error at position closing request: \"%1%\".",
          ex.what());
    }
  }
};

aa::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const ptr::ptree &conf)
    : Base(context, typeId, "ArbitrageAdvisor", instanceName, conf),
      m_pimpl(boost::make_unique<Implementation>(*this, conf)) {}

aa::Strategy::~Strategy() = default;

sig::scoped_connection aa::Strategy::SubscribeToAdvice(
    const boost::function<void(const Advice &)> &slot) {
  const auto &result = m_pimpl->m_adviceSignal.connect(slot);
  {
    const auto lock = LockForOtherThreads();
    m_pimpl->RecheckSignalAsync();
  }
  return result;
}

sig::scoped_connection aa::Strategy::SubscribeToTradingSignalCheckErrors(
    const boost::function<void(const std::vector<std::string> &)> &slot) {
  return m_pimpl->m_tradingSignalCheckErrorsSignal.connect(slot);
}

sig::scoped_connection aa::Strategy::SubscribeToBlocking(
    const boost::function<void(const std::string *)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

void aa::Strategy::SetMinPriceDifferenceRatioToAdvice(
    const Double &minPriceDifferenceRatio) {
  const auto lock = LockForOtherThreads();
  GetTradingLog().Write(
      "{'setup': {'advising': {'ratio': '%1%->%2%'}}}",
      [this, &minPriceDifferenceRatio](TradingRecord &record) {
        record % m_pimpl->m_minPriceDifferenceRatioToAdvice  // 1
            % minPriceDifferenceRatio;                       // 2
      });
  if (m_pimpl->m_minPriceDifferenceRatioToAdvice == minPriceDifferenceRatio) {
    return;
  }
  m_pimpl->m_minPriceDifferenceRatioToAdvice = minPriceDifferenceRatio;
  m_pimpl->RecheckSignalAsync();
}

const Double &aa::Strategy::GetMinPriceDifferenceRatioToAdvice() const {
  return m_pimpl->m_minPriceDifferenceRatioToAdvice;
}

bool aa::Strategy::IsLowestSpreadEnabled() const {
  return m_pimpl->m_isLowestSpreadEnabled;
}
const Double &aa::Strategy::GetLowestSpreadRatio() const {
  return m_pimpl->m_lowestSpreadRatio;
}

bool aa::Strategy::IsStopLossEnabled() const {
  return m_pimpl->m_isStopLossEnabled;
}
const pt::time_duration &aa::Strategy::GetStopLossDelay() const {
  return m_pimpl->m_stopLossDelay;
}

void aa::Strategy::ForEachSecurity(
    const Symbol &symbol, const boost::function<void(Security &)> &callback) {
  for (auto &security : m_pimpl->m_symbols[symbol]) {
    callback(*security.security);
  }
}

void aa::Strategy::OnSecurityStart(Security &security, Security::Request &) {
  Assert(std::find_if(m_pimpl->m_symbols[security.GetSymbol()].cbegin(),
                      m_pimpl->m_symbols[security.GetSymbol()].cend(),
                      [&security](const AdviceSecuritySignal &stored) {
                        return stored.security == &security ||
                               stored.isBestBid || stored.isBestAsk;
                      }) == m_pimpl->m_symbols[security.GetSymbol()].cend());
  m_pimpl->m_symbols[security.GetSymbol()].emplace_back(
      AdviceSecuritySignal{&security});
}

void aa::Strategy::SetTradingSettings(TradingSettings &&settings) {
  const auto lock = LockForOtherThreads();
  GetTradingLog().Write(
      "{'setup': {'trading': {'isEnabled': '%5%->%6%', 'ratio': '%1%->%2%', "
      "'maxQty': '%3%->%4%}}}",
      [this, &settings](TradingRecord &record) {
        record % m_pimpl->m_tradingSettings.minPriceDifferenceRatio  // 1
            % settings.minPriceDifferenceRatio                       // 2
            % m_pimpl->m_tradingSettings.maxQty                      // 3
            % settings.maxQty                                        // 4
            % (m_pimpl->m_tradingSettings.isEnabled ? "yes" : "no")  // 5
            % (settings.isEnabled ? "yes" : "no");                   // 6
      });
  const auto shouldBeRechecked =
      m_pimpl->m_tradingSettings.minPriceDifferenceRatio !=
      settings.minPriceDifferenceRatio;

  m_pimpl->m_tradingSettings = std::move(settings);

  if (m_pimpl->m_tradingSettings.isEnabled) {
    if (shouldBeRechecked) {
      m_pimpl->RecheckSignalAsync();
    }
  } else {
    m_pimpl->m_errors.clear();
  }
}

const aa::Strategy::TradingSettings &aa::Strategy::GetTradingSettings() const {
  return m_pimpl->m_tradingSettings;
}

void aa::Strategy::OnLevel1Update(Security &security,
                                  const Milestones &delayMeasurement) {
  m_pimpl->CheckAutoTradingSignal(security, delayMeasurement);
}

void aa::Strategy::OnPositionUpdate(Position &position) {
  try {
    m_pimpl->m_controller.OnUpdate(position);
    if (position.IsCompleted()) {
      m_pimpl->CheckAutoTradingSignal(position.GetSecurity(), Milestones());
    }
  } catch (const CommunicationError &ex) {
    GetLog().Warn("Communication error at position update handling: \"%1%\".",
                  ex.what());
  }
}

void aa::Strategy::OnPostionsCloseRequest() {
  m_pimpl->m_controller.OnCloseAllRequest(*this);
}

bool aa::Strategy::OnBlocked(const std::string *reason) noexcept {
  try {
    m_pimpl->m_blockSignal(reason);
  } catch (...) {
    AssertFailNoException();
    return Base::OnBlocked(reason);
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateStrategy(Context &context,
                                               const std::string &instanceName,
                                               const ptr::ptree &conf) {
  return boost::make_unique<aa::Strategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
