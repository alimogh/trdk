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
#include "TradingLib/PositionController.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace aa = trdk::Strategies::ArbitrageAdvisor;
namespace sig = boost::signals2;

////////////////////////////////////////////////////////////////////////////////

class aa::Strategy::Implementation : private boost::noncopyable {
 public:
  PositionController m_controller;
  sig::signal<void(const Advice &)> m_adviceSignal;

  Double m_minPriceDifferenceRatioToAdvice;
  boost::optional<Double> m_minPriceDifferenceRatioToTrade;

  boost::unordered_map<Symbol, std::vector<Advice::SecuritySignal>> m_symbols;

  explicit Implementation(aa::Strategy &self)
      : m_controller(self), m_minPriceDifferenceRatioToAdvice(0) {}

  void CheckSignal(Security &updatedSecurity,
                   std::vector<Advice::SecuritySignal> &allSecurities,
                   const Milestones &) {
    typedef std::pair<Price, Advice::SecuritySignal *> PriceItem;

    std::vector<PriceItem> bids;
    std::vector<PriceItem> asks;

    bids.reserve(allSecurities.size());
    asks.reserve(allSecurities.size());

    Price updatedSecurityBidPrice = std::numeric_limits<double>::quiet_NaN();
    Price updatedSecurityAskPrice = std::numeric_limits<double>::quiet_NaN();
    for (auto &security : allSecurities) {
      security.isBidSignaled = security.isAskSignaled = false;
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
    bool isSignaled;
    if (!bids.empty() && !asks.empty()) {
      spread = bids.front().first - asks.front().first;
      spreadRatio =
          RoundByPrecision((100 / (asks.front().first / spread)) / 100, 100);
      isSignaled = spreadRatio >= m_minPriceDifferenceRatioToAdvice;
    } else {
      spread = spreadRatio = std::numeric_limits<double>::quiet_NaN();
      isSignaled = false;
    }

    for (auto it = bids.begin();
         it != bids.end() && it->first == bids.front().first; ++it) {
      Assert(!it->second->isBidSignaled);
      it->second->isBidSignaled = true;
    }
    for (auto it = asks.begin();
         it != asks.end() && it->first == asks.front().first; ++it) {
      Assert(!it->second->isAskSignaled);
      it->second->isAskSignaled = true;
    }

    m_adviceSignal(
        Advice{&updatedSecurity,
               updatedSecurity.GetLastMarketDataTime(),
               {updatedSecurityBidPrice, updatedSecurity.GetBidQtyValue()},
               {updatedSecurityAskPrice, updatedSecurity.GetAskQtyValue()},
               spread,
               spreadRatio,
               isSignaled,
               allSecurities});
  }

  void RecheckSignal() {
    for (auto &symbol : m_symbols) {
      for (const Advice::SecuritySignal &security : symbol.second) {
        CheckSignal(*security.security, symbol.second, Milestones());
      }
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
      "setup advising\tratio=%1$.8f->%2$.8f",
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
                               stored.isBidSignaled || stored.isAskSignaled;
                      }) == m_pimpl->m_symbols[security.GetSymbol()].cend());
  m_pimpl->m_symbols[security.GetSymbol()].emplace_back(
      Advice::SecuritySignal{&security});
}

void aa::Strategy::ActivateAutoTrading(const Double &minPriceDifferenceRatio) {
  if (m_pimpl->m_minPriceDifferenceRatioToTrade) {
    GetTradingLog().Write(
        "setup trading\tratio=%1$.8f->%2$.8f",
        [this, &minPriceDifferenceRatio](TradingRecord &record) {
          record % *m_pimpl->m_minPriceDifferenceRatioToTrade  // 1
              % minPriceDifferenceRatio;                       // 2
        });
  } else {
    GetTradingLog().Write(
        "setup trading\tratio=none->%1$.8f",
        [this, &minPriceDifferenceRatio](TradingRecord &record) {
          record % minPriceDifferenceRatio;  // 1
        });
  }
  if (m_pimpl->m_minPriceDifferenceRatioToTrade == minPriceDifferenceRatio) {
    return;
  }
  m_pimpl->m_minPriceDifferenceRatioToTrade == minPriceDifferenceRatio;
  m_pimpl->RecheckSignal();
}

const boost::optional<Double>
    &aa::Strategy::GetAutoTradingMinPriceDifferenceRatio() const {
  return m_pimpl->m_minPriceDifferenceRatioToTrade;
}
void aa::Strategy::DeactivateAutoTrading() {
  if (m_pimpl->m_minPriceDifferenceRatioToTrade) {
    GetTradingLog().Write("setup trading\tratio=%1$.8f->none",
                          [this](TradingRecord &record) {
                            record % *m_pimpl->m_minPriceDifferenceRatioToTrade;
                          });
  } else {
    GetTradingLog().Write("setup trading\tratio=none->none");
  }
  m_pimpl->m_minPriceDifferenceRatioToTrade = boost::none;
}

void aa::Strategy::OnLevel1Update(Security &security,
                                  const Milestones &delayMeasurement) {
  if (m_pimpl->m_adviceSignal.num_slots() == 0 &&
      !m_pimpl->m_minPriceDifferenceRatioToTrade) {
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
