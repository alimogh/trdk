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
#include "OperationContext.hpp"
#include "PositionController.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace aa = trdk::Strategies::ArbitrageAdvisor;
namespace sig = boost::signals2;

////////////////////////////////////////////////////////////////////////////////

class aa::Strategy::Implementation : private boost::noncopyable {
 public:
  aa::Strategy &m_self;
  PositionController m_controller;
  sig::signal<void(const Advice &)> m_adviceSignal;

  Double m_minPriceDifferenceRatioToAdvice;
  boost::optional<TradingSettings> m_tradingSettings;

  boost::unordered_map<Symbol, std::vector<Advice::SecuritySignal>> m_symbols;

  std::pair<Security *, Security *> m_lastTradeTargets;

  explicit Implementation(aa::Strategy &self)
      : m_self(self),
        m_controller(m_self),
        m_minPriceDifferenceRatioToAdvice(0),
        m_lastTradeTargets({}) {}

  void CheckSignal(Security &updatedSecurity,
                   std::vector<Advice::SecuritySignal> &allSecurities,
                   const Milestones &delayMeasurement) {
    typedef std::pair<Price, Advice::SecuritySignal *> PriceItem;

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
      spread = bids.front().first - asks.front().first;
      spreadRatio =
          RoundByPrecision((100 / (asks.front().first / spread)) / 100, 100);
      if (m_tradingSettings &&
          spreadRatio >= m_tradingSettings->minPriceDifferenceRatio) {
        Trade(*bids.front().second->security, *asks.front().second->security,
              m_tradingSettings->maxQty, spreadRatio, delayMeasurement);
      } else {
        StopTrading(spreadRatio);
      }
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

  void RecheckSignal() {
    for (auto &symbol : m_symbols) {
      for (const Advice::SecuritySignal &security : symbol.second) {
        CheckSignal(*security.security, symbol.second, Milestones());
      }
    }
  }

  void Trade(Security &sellTarget,
             Security &buyTarget,
             const Qty &maxQty,
             const Double &spreadRatio,
             const Milestones &delayMeasurement) {
    if (!m_self.GetPositions().IsEmpty()) {
      for (auto &strategyPosition : m_self.GetPositions()) {
        if (&strategyPosition.GetSecurity() != &sellTarget &&
            &strategyPosition.GetSecurity() != &buyTarget) {
          m_self.GetTradingLog().Write(
              "{'trade': {'change': {'sell': {'exchange': '%1%', 'bid': "
              "%2$.8f, 'ask': %3$.8f}, 'buy': {'exchange': '%4%', 'bid': "
              "%5$.8f, 'ask': %6$.8f}}, 'spread': %7$.3f}}",
              [&](TradingRecord &record) {
                OperationContext operation(sellTarget, buyTarget, maxQty);
                record %
                    boost::cref(operation.GetTradingSystem(m_self, sellTarget)
                                    .GetInstanceName())  // 1
                    % sellTarget.GetBidPriceValue()      // 2
                    % sellTarget.GetAskPriceValue()      // 3
                    % boost::cref(operation.GetTradingSystem(m_self, buyTarget)
                                      .GetInstanceName())  // 4
                    % buyTarget.GetBidPriceValue()         // 5
                    % buyTarget.GetAskPriceValue()         // 6
                    % spreadRatio;                         // 7
              });
          StopTrading(spreadRatio);
          return;
        }
      }
      return;
    }
    if (m_lastTradeTargets.first == &sellTarget &&
        m_lastTradeTargets.second == &buyTarget) {
      return;
    }
    auto operation =
        boost::make_shared<OperationContext>(sellTarget, buyTarget, maxQty);
    m_self.GetTradingLog().Write(
        "{'trade': {'new': {'sell': {'exchange': '%1%', 'bid': %2$.8f, 'ask': "
        "%3$.8f}, 'buy': {'exchange': '%4%', 'bid': %5$.8f, 'ask': %6$.8f}}, "
        "'spread': %7$.3f}}",
        [&](TradingRecord &record) {
          record % boost::cref(operation->GetTradingSystem(m_self, sellTarget)
                                   .GetInstanceName())  // 1
              % sellTarget.GetBidPriceValue()           // 2
              % sellTarget.GetAskPriceValue()           // 3
              % boost::cref(operation->GetTradingSystem(m_self, buyTarget)
                                .GetInstanceName())  // 4
              % buyTarget.GetBidPriceValue()         // 5
              % buyTarget.GetAskPriceValue()         // 6
              % spreadRatio;                         // 7
        });
    m_controller.OpenPosition(operation, sellTarget, delayMeasurement);
    m_controller.OpenPosition(operation, buyTarget, delayMeasurement);
    m_lastTradeTargets = {&sellTarget, &buyTarget};
  }

  void StopTrading(const Double &spreadRatio) {
    m_lastTradeTargets = {};
    if (m_self.GetPositions().IsEmpty()) {
      return;
    }
    m_self.GetTradingLog().Write("{'trade': {'stop'}, 'spread': %1$.3f}}",
                                 [&](TradingRecord &record) {
                                   record % spreadRatio;  // 1
                                 });
    for (Position &position : m_self.GetPositions()) {
      m_controller.ClosePosition(position, CLOSE_REASON_SIGNAL);
    }
  }
};

aa::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf)
    : Base(context,
           "{39FBFFDA-10D7-462D-BA82-0D8BA9CA7A09}",
           "ArbitrageAdvisor",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

aa::Strategy::~Strategy() = default;

sig::scoped_connection aa::Strategy::SubscribeToAdvice(
    const boost::function<void(const Advice &)> &slot) {
  const auto &result = m_pimpl->m_adviceSignal.connect(slot);
  m_pimpl->RecheckSignal();
  return result;
}

void aa::Strategy::SetupAdvising(const Double &minPriceDifferenceRatio) const {
  GetTradingLog().Write(
      "{'setup': {'advising': {'ratio': '%1$.8f->%2$.8f'}}}",
      [this, &minPriceDifferenceRatio](TradingRecord &record) {
        record % m_pimpl->m_minPriceDifferenceRatioToAdvice  // 1
            % minPriceDifferenceRatio;                       // 2
      });
  if (m_pimpl->m_minPriceDifferenceRatioToAdvice == minPriceDifferenceRatio) {
    return;
  }
  m_pimpl->m_minPriceDifferenceRatioToAdvice = minPriceDifferenceRatio;
  m_pimpl->RecheckSignal();
}

void aa::Strategy::OnSecurityStart(Security &security, Security::Request &) {
  Assert(std::find_if(m_pimpl->m_symbols[security.GetSymbol()].cbegin(),
                      m_pimpl->m_symbols[security.GetSymbol()].cend(),
                      [&security](const Advice::SecuritySignal &stored) {
                        return stored.security == &security ||
                               stored.isBestBid || stored.isBestAsk;
                      }) == m_pimpl->m_symbols[security.GetSymbol()].cend());
  m_pimpl->m_symbols[security.GetSymbol()].emplace_back(
      Advice::SecuritySignal{&security});
}

void aa::Strategy::ActivateAutoTrading(TradingSettings &&settings) {
  bool shouldRechecked = true;
  if (m_pimpl->m_tradingSettings) {
    GetTradingLog().Write(
        "{'setup': {'trading': {'ratio': '%1$.8f->%2$.8f', 'maxQty': "
        "'%3$.8f->%4$.8f}}}",
        [this, &settings](TradingRecord &record) {
          record % m_pimpl->m_tradingSettings->minPriceDifferenceRatio  // 1
              % settings.minPriceDifferenceRatio                        // 2
              % m_pimpl->m_tradingSettings->maxQty                      // 3
              % settings.maxQty;                                        // 4
        });
    shouldRechecked = m_pimpl->m_tradingSettings->minPriceDifferenceRatio !=
                      settings.minPriceDifferenceRatio;
  } else {
    GetTradingLog().Write(
        "{'setup': {'trading': {'ratio': 'null->%1$.8f', 'maxQty': "
        "'null->%2$.8f'}}}",
        [this, &settings](TradingRecord &record) {
          record % settings.minPriceDifferenceRatio  // 1
              % settings.maxQty;                     // 2
        });
  }
  m_pimpl->m_tradingSettings = std::move(settings);
  m_pimpl->RecheckSignal();
}

const boost::optional<aa::Strategy::TradingSettings>
    &aa::Strategy::GetAutoTradingSettings() const {
  return m_pimpl->m_tradingSettings;
}
void aa::Strategy::DeactivateAutoTrading() {
  if (m_pimpl->m_tradingSettings) {
    GetTradingLog().Write(
        "{'setup': {'trading': {'ratio': '%1$.8f->null', 'maxQty': "
        "'%2$.8f->null'}}}",
        [this](TradingRecord &record) {
          record % m_pimpl->m_tradingSettings->minPriceDifferenceRatio  // 1
              % m_pimpl->m_tradingSettings->maxQty;                     // 2
        });
  } else {
    GetTradingLog().Write(
        "{'setup': {'trading': {'ratio': 'null->null', 'maxQty': "
        "'null->null'}}}");
  }
  m_pimpl->m_tradingSettings = boost::none;
}

void aa::Strategy::OnLevel1Update(Security &security,
                                  const Milestones &delayMeasurement) {
  if (m_pimpl->m_adviceSignal.num_slots() == 0 && !m_pimpl->m_tradingSettings) {
    return;
  }
  AssertLt(0, m_pimpl->m_adviceSignal.num_slots());
  m_pimpl->CheckSignal(security, m_pimpl->m_symbols[security.GetSymbol()],
                       delayMeasurement);
}

void aa::Strategy::OnPositionUpdate(Position &position) {
  m_pimpl->m_controller.OnPositionUpdate(position);
}

void aa::Strategy::OnPostionsCloseRequest() {
  m_pimpl->m_controller.OnPostionsCloseRequest();
}
////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::Strategy> CreateStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  return boost::make_shared<aa::Strategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
