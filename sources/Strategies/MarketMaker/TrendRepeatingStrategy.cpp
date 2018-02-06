/*******************************************************************************
 *   Created: 2018/01/31 20:52:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TrendRepeatingStrategy.hpp"
#include "TrendRepeatingController.hpp"
#include "TrendRepeatingOperation.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace Lib::TimeMeasurement;
using namespace trdk::Strategies::MarketMaker;

namespace accs = boost::accumulators;
namespace ma = trdk::Lib::Accumulators::MovingAverage;
namespace sig = boost::signals2;
namespace tl = trdk::TradingLib;

using tl::StopLossShare;
using tl::TakeProfit;

namespace {
struct Ma {
  const char *name;
  bool isReported;
  size_t numberOfPeriods;
  std::unique_ptr<ma::Exponential> stat;

  explicit Ma(const char *name, size_t numberOfPeriods)
      : name(name), isReported(false), numberOfPeriods(numberOfPeriods) {
    Reset();
  }

  bool IsFilled() const {
    return accs::rolling_count(*stat) >= numberOfPeriods;
  }

  void Reset() {
    stat.reset(new ma::Exponential(accs::tag::rolling_window::window_size =
                                       numberOfPeriods));
  }
};

class Trend : public tl::Trend {
 public:
  bool Update(const ma::Exponential &slowMa, const ma::Exponential &fastMa) {
    return SetIsRising(accs::ema(slowMa) < accs::ema(fastMa));
  }
};
}  // namespace

class TrendRepeatingStrategy::Implementation : private boost::noncopyable {
 public:
  TrendRepeatingStrategy &m_self;

  bool m_isTradingEnabled;

  Qty m_positionSize;

  Ma m_fastMa;
  Ma m_slowMa;

  boost::shared_ptr<TakeProfit::Params> m_takeProfit;
  boost::shared_ptr<StopLossShare::Params> m_stopLoss;

  boost::circular_buffer<Price> m_marketDataBuffer;

  sig::signal<void(const std::string &)> m_eventsSignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  TrendRepeatingController m_controller;

  Trend m_trend;

  boost::unordered_map<Security *, bool> m_securities;

 public:
  explicit Implementation(TrendRepeatingStrategy &self)
      : m_self(self),
        m_isTradingEnabled(false),
        m_positionSize(.01),
        m_fastMa("fast", 12),
        m_slowMa("slow", 26),
        m_takeProfit(boost::make_shared<TakeProfit::Params>(.002, 0)),
        m_stopLoss(boost::make_shared<StopLossShare::Params>(5.0 / 100)),
        m_marketDataBuffer(
            std::max(m_fastMa.numberOfPeriods, m_slowMa.numberOfPeriods)) {}

  void SetNumberOfMaPeriods(Ma &ma, size_t newNumberOfPeriods) {
    const auto lock = m_self.LockForOtherThreads();
    if (ma.numberOfPeriods == newNumberOfPeriods) {
      return;
    }
    const auto prevNumberOfPeriods = ma.numberOfPeriods;
    const auto prevFilledSize = accs::rolling_count(*ma.stat);
    ma.numberOfPeriods = newNumberOfPeriods;
    ma.Reset();
    for (auto it = m_marketDataBuffer.size() <= newNumberOfPeriods
                       ? m_marketDataBuffer.begin()
                       : m_marketDataBuffer.end() - newNumberOfPeriods;
         it != m_marketDataBuffer.end(); ++it) {
      (*ma.stat)(*it);
    }
    ma.isReported = false;

    m_self.GetTradingLog().Write("%1% MA size: %2% (%3%) -> %4% (%5%)",
                                 [&](TradingRecord &record) {
                                   record % ma.name                      // 1
                                       % prevNumberOfPeriods             // 2
                                       % prevFilledSize                  // 3
                                       % ma.numberOfPeriods              // 4
                                       % accs::rolling_count(*ma.stat);  // 5
                                 });

    if (m_marketDataBuffer.capacity() < newNumberOfPeriods) {
      m_self.GetTradingLog().Write(
          "MA buffer size: %1% -> %2%", [&](TradingRecord &record) {
            record % m_marketDataBuffer.capacity()  // 1
                % newNumberOfPeriods;               // 2
          });
      m_marketDataBuffer.set_capacity(newNumberOfPeriods);
    }
  }
  size_t GetNumberOfMaPeriods(const Ma &ma) const {
    const auto lock = m_self.LockForOtherThreads();
    return ma.numberOfPeriods;
  }

  void CheckSignal(Security &sourceSecurity,
                   bool isTradingSystemEnabled,
                   const Milestones &delayMeasurement) {
    if (!m_slowMa.IsFilled() || !m_fastMa.IsFilled()) {
      return;
    }
    if (!m_trend.Update(*m_slowMa.stat, *m_fastMa.stat)) {
      return;
    }
    if ((m_self.GetPositions().IsEmpty() && !m_isTradingEnabled) ||
        !isTradingSystemEnabled) {
      return;
    }

    m_self.GetTradingLog().Write(
        "trend\tchanged\tslow_ema=%1%\tfast_ema=%2%\ttrend=%3%",
        [&](TradingRecord &record) {
          record % accs::ema(*m_slowMa.stat)                  // 1
              % accs::ema(*m_fastMa.stat)                     // 2
              % (m_trend.IsRising() ? "rising" : "falling");  // 3
        });
    m_self.RaiseEvent("Trend changed to \"" +
                      std::string(m_trend.IsRising() ? "rising" : "falling") +
                      "\".");

    const bool isLong = m_trend.IsRising();

    const auto &bestTargetChecker =
        tl::BestSecurityCheckerForOrder::Create(m_self, isLong, m_positionSize);
    for (const auto &security : m_securities) {
      bestTargetChecker->Check(*security.first);
    }
    auto *const targetSecurity = bestTargetChecker->GetSuitableSecurity();
    if (!targetSecurity) {
      m_self.GetLog().Warn(
          "Failed to find suitable security for the new position (source "
          "security is \"%1%\").",
          sourceSecurity);
      m_self.RaiseEvent("Failed to find suitable exchange to open " +
                        std::string(isLong ? "long" : "short") + " position.");
      return;
    }

    try {
      m_controller.OnSignal(
          boost::make_shared<TrendRepeatingOperation>(
              m_self, m_positionSize, isLong, m_takeProfit, m_stopLoss),
          0, *targetSecurity, delayMeasurement);
    } catch (const CommunicationError &ex) {
      m_self.GetLog().Debug(
          "Communication error at position update handling: \"%1%\".",
          ex.what());
    }
  }
};

TrendRepeatingStrategy::TrendRepeatingStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf)
    : Base(context,
           "{C9C4282A-C620-45DA-9071-A6F9E5224BE9}",
           "MaTrendRepeatingMarketMaker",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

TrendRepeatingStrategy::~TrendRepeatingStrategy() = default;

void TrendRepeatingStrategy::OnSecurityStart(Security &security,
                                             Security::Request &request) {
  if (!m_pimpl->m_securities.empty() &&
      m_pimpl->m_securities.begin()->first->GetSymbol() !=
          security.GetSymbol()) {
    throw Exception("Strategy works only with one symbol, but more is set");
  }
  Verify(m_pimpl->m_securities.emplace(std::make_pair(&security, true)).second);
  Base::OnSecurityStart(security, request);
}

void TrendRepeatingStrategy::OnLevel1Update(
    Security &security, const Milestones &delayMeasurement) {
  const auto &securityIt = m_pimpl->m_securities.find(&security);
  Assert(securityIt != m_pimpl->m_securities.cend());
  if (securityIt == m_pimpl->m_securities.cend()) {
    return;
  }

  const auto &bid = security.GetBidPriceValue();
  const auto &ask = security.GetAskPriceValue();
  const auto &spread = ask - bid;
  const auto &statValue = bid + (spread / 2);
  m_pimpl->m_marketDataBuffer.push_back(statValue);

  (*m_pimpl->m_fastMa.stat)(statValue);
  (*m_pimpl->m_slowMa.stat)(statValue);

  if (!m_pimpl->m_slowMa.isReported || !m_pimpl->m_fastMa.isReported) {
    if (m_pimpl->m_slowMa.IsFilled() && m_pimpl->m_slowMa.IsFilled()) {
      RaiseEvent("Both MA are filled, starting signal checks...");
      m_pimpl->m_slowMa.isReported = m_pimpl->m_fastMa.isReported = true;
    }
    if (!m_pimpl->m_slowMa.isReported && m_pimpl->m_slowMa.IsFilled()) {
      RaiseEvent("Slow MA is filled, waiting for fast MA filling.");
      m_pimpl->m_slowMa.isReported = true;
    }
    if (!m_pimpl->m_fastMa.isReported && m_pimpl->m_fastMa.IsFilled()) {
      RaiseEvent("Fast MA is filled, waiting for slow MA filling.");
      m_pimpl->m_fastMa.isReported = true;
    }
  }

  m_pimpl->CheckSignal(security, securityIt->second, delayMeasurement);
}

void TrendRepeatingStrategy::OnPositionUpdate(Position &position) {
  try {
    m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Debug("Communication error at position update handling: \"%1%\".",
                   ex.what());
  }
}

void TrendRepeatingStrategy::OnPostionsCloseRequest() {
  m_pimpl->m_controller.OnPostionsCloseRequest(*this);
}

bool TrendRepeatingStrategy::OnBlocked(const std::string *reason) noexcept {
  try {
    {
      boost::format message("Blocked by error: \"%1%\".");
      if (reason) {
        message % *reason;
      } else {
        message % "Unknown error";
      }
      RaiseEvent(message.str());
    }
    m_pimpl->m_blockSignal(reason);
  } catch (...) {
    AssertFailNoException();
    return Base::OnBlocked(reason);
  }
  return false;
}

void TrendRepeatingStrategy::SetPositionSize(const Qty &size) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_positionSize = size;
}
Qty TrendRepeatingStrategy::GetPositionSize() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_positionSize;
}

void TrendRepeatingStrategy::EnableTrading(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_isTradingEnabled == isEnabled) {
    return;
  }
  m_pimpl->m_isTradingEnabled = isEnabled;
  if (isEnabled) {
    if (m_pimpl->m_securities.empty()) {
      RaiseEvent(
          "Failed to enable trading as no one selected exchange supports "
          "specified symbol.");
      return;
    }
    m_pimpl->m_controller.EnableClosing(true);
  }
  GetTradingLog().Write("%1% trading", [isEnabled](TradingRecord &record) {
    record % (isEnabled ? "enabled" : "disabled");
  });
}
bool TrendRepeatingStrategy::IsTradingEnabled() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_isTradingEnabled;
}

void TrendRepeatingStrategy::EnableActivePositionsControl(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_controller.IsClosedEnabled() == isEnabled) {
    return;
  }
  if (!isEnabled && m_pimpl->m_isTradingEnabled) {
    return;
  }
  m_pimpl->m_controller.EnableClosing(isEnabled);
  GetTradingLog().Write("%1% position control",
                        [isEnabled](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
}
bool TrendRepeatingStrategy::IsActivePositionsControlEnabled() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_controller.IsClosedEnabled();
}

void TrendRepeatingStrategy::SetNumberOfFastMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(m_pimpl->m_fastMa, numberOfPeriods);
}
size_t TrendRepeatingStrategy::GetNumberOfFastMaPeriods() const {
  return m_pimpl->GetNumberOfMaPeriods(m_pimpl->m_fastMa);
}
void TrendRepeatingStrategy::SetNumberOfSlowMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(m_pimpl->m_slowMa, numberOfPeriods);
}
size_t TrendRepeatingStrategy::GetNumberOfSlowMaPeriods() const {
  return m_pimpl->GetNumberOfMaPeriods(m_pimpl->m_slowMa);
}

void TrendRepeatingStrategy::SetStopLoss(const Double &stopLoss) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_stopLoss->GetMaxLossShare() == stopLoss) {
    return;
  }
  GetTradingLog().Write("stop-loss: %1% -> %2%", [&](TradingRecord &record) {
    record % m_pimpl->m_stopLoss->GetMaxLossShare()  // 1
        % stopLoss;                                  // 2
  });
  *m_pimpl->m_stopLoss = StopLossShare::Params{stopLoss};
}
Double TrendRepeatingStrategy::GetStopLoss() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_stopLoss->GetMaxLossShare();
}
void TrendRepeatingStrategy::SetTakeProfit(const Double &takeProfit) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_takeProfit->GetMinProfitPerLotToActivate() == takeProfit) {
    return;
  }
  GetTradingLog().Write("take profit: %1% -> %2%", [&](TradingRecord &record) {
    record % m_pimpl->m_takeProfit->GetMinProfitPerLotToActivate()  // 1
        % takeProfit;                                               // 2
  });
  *m_pimpl->m_takeProfit = TakeProfit::Params{takeProfit, 0};
}
Double TrendRepeatingStrategy::GetTakeProfit() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_takeProfit->GetMinProfitPerLotToActivate();
}

sig::scoped_connection TrendRepeatingStrategy::SubscribeToEvents(
    const boost::function<void(const std::string &)> &slot) {
  return m_pimpl->m_eventsSignal.connect(slot);
}

sig::scoped_connection TrendRepeatingStrategy::SubscribeToBlocking(
    const boost::function<void(const std::string *)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

const tl::Trend &TrendRepeatingStrategy::GetTrend() const {
  return m_pimpl->m_trend;
}

void TrendRepeatingStrategy::ForEachSecurity(
    const boost::function<void(Security &)> &callback) {
  for (auto &security : m_pimpl->m_securities) {
    callback(*security.first);
  }
}

void TrendRepeatingStrategy::RaiseEvent(const std::string &message) {
  m_pimpl->m_eventsSignal(message);
}

void TrendRepeatingStrategy::EnableTradingSystem(size_t tradingSystemIndex,
                                                 bool isEnabled) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() == tradingSystemIndex &&
        security.second != isEnabled) {
      security.second = isEnabled;
      GetTradingLog().Write("%1% %2%", [&](TradingRecord &record) {
        record % *security.first                           // 1
            % (security.second ? "enabled" : "disabled");  // 2
      });
    }
  }
}

boost::tribool TrendRepeatingStrategy::IsTradingSystemEnabled(
    size_t tradingSystemIndex) const {
  for (const auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() == tradingSystemIndex) {
      return security.second;
    }
  }
  return boost::indeterminate;
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateMaTrendRepeatingStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  return boost::make_unique<TrendRepeatingStrategy>(context, instanceName,
                                                    conf);
}

////////////////////////////////////////////////////////////////////////////////
