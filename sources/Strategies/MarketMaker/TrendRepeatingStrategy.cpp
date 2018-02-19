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
using tl::TakeProfitShare;

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

struct Subscribtion {
  bool isEnabled;
  Ma fastMa;
  Ma slowMa;
  boost::circular_buffer<Price> marketDataBuffer;
  Trend trend;

  explicit Subscribtion(size_t fastMaSize, size_t slowMaSize)
      : isEnabled(true),
        fastMa("fast", fastMaSize),
        slowMa("slow", slowMaSize),
        marketDataBuffer(
            std::max(fastMa.numberOfPeriods, slowMa.numberOfPeriods)) {}
};
}  // namespace

class TrendRepeatingStrategy::Implementation : private boost::noncopyable {
 public:
  TrendRepeatingStrategy &m_self;

  Qty m_positionSize;

  size_t m_fastMaSize;
  size_t m_slowMaSize;

  boost::shared_ptr<TakeProfitShare::Params> m_takeProfit;
  boost::shared_ptr<StopLossShare::Params> m_stopLoss;

  sig::signal<void(const std::string &)> m_eventsSignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  TrendRepeatingController m_controller;

  boost::unordered_map<const Security *, boost::shared_ptr<Subscribtion>>
      m_securities;

  bool m_isStopped;

 public:
  explicit Implementation(TrendRepeatingStrategy &self)
      : m_self(self),
        m_positionSize(.01),
        m_fastMaSize(12),
        m_slowMaSize(26),
        m_takeProfit(
            boost::make_shared<TakeProfitShare::Params>(3.0 / 100, .75 / 100)),
        m_stopLoss(boost::make_shared<StopLossShare::Params>(15.0 / 100)),
        m_isStopped(false) {}

  template <typename GetMa>
  void SetNumberOfMaPeriods(const GetMa &getMa,
                            size_t &currentNumberOfPeriods,
                            size_t newNumberOfPeriods) {
    const auto lock = m_self.LockForOtherThreads();
    for (auto &security : m_securities) {
      Subscribtion &subscribtion = *security.second;
      Ma &ma = getMa(subscribtion);

      if (ma.numberOfPeriods == newNumberOfPeriods) {
        return;
      }

      const auto prevNumberOfPeriods = ma.numberOfPeriods;
      const auto prevFilledSize = accs::rolling_count(*ma.stat);
      ma.numberOfPeriods = newNumberOfPeriods;
      ma.Reset();

      for (auto it =
               subscribtion.marketDataBuffer.size() <= newNumberOfPeriods
                   ? subscribtion.marketDataBuffer.begin()
                   : subscribtion.marketDataBuffer.end() - newNumberOfPeriods;
           it != subscribtion.marketDataBuffer.end(); ++it) {
        (*ma.stat)(*it);
      }

      ma.isReported = false;

      m_self.GetTradingLog().Write(
          "%1% MA size for \"%2%\": %3% (%4%) -> %5% (%6%)",
          [&](TradingRecord &record) {
            record % ma.name                      // 1
                % *security.first                 // 2
                % prevNumberOfPeriods             // 3
                % prevFilledSize                  // 4
                % ma.numberOfPeriods              // 5
                % accs::rolling_count(*ma.stat);  // 6
          });

      if (subscribtion.marketDataBuffer.capacity() < newNumberOfPeriods) {
        m_self.GetTradingLog().Write(
            "MA buffer size: %1% -> %2%", [&](TradingRecord &record) {
              record % subscribtion.marketDataBuffer.capacity()  // 1
                  % newNumberOfPeriods;                          // 2
            });
        subscribtion.marketDataBuffer.set_capacity(newNumberOfPeriods);
      }
    }
    currentNumberOfPeriods = newNumberOfPeriods;
  }

  void CheckSignal(Security &sourceSecurity,
                   Subscribtion &subscribtion,
                   const Milestones &delayMeasurement) {
    if (!subscribtion.slowMa.IsFilled() || !subscribtion.fastMa.IsFilled()) {
      return;
    }
    if (!subscribtion.trend.Update(*subscribtion.slowMa.stat,
                                   *subscribtion.fastMa.stat)) {
      return;
    }
    if (!subscribtion.isEnabled) {
      return;
    }

    const auto &tradingSystem =
        m_self.GetTradingSystem(sourceSecurity.GetSource().GetIndex())
            .GetInstanceName();

    m_self.GetTradingLog().Write(
        "trend\tchanged\t%1%\tslow_ema=%2%\tfast_ema=%3%\ttrend=%4%",
        [&](TradingRecord &record) {
          record % sourceSecurity                                        // 1
              % accs::ema(*subscribtion.slowMa.stat)                     // 2
              % accs::ema(*subscribtion.fastMa.stat)                     // 3
              % (subscribtion.trend.IsRising() ? "rising" : "falling");  // 4
        });
    m_self.RaiseEvent(tradingSystem + " changed trend to \"" +
                      (subscribtion.trend.IsRising() ? "rising" : "falling") +
                      "\".");

    if (!m_controller.IsOpeningEnabled()) {
      return;
    }

    try {
      m_controller.OnSignal(
          boost::make_shared<TrendRepeatingOperation>(
              m_self, m_positionSize, subscribtion.trend.IsRising(),
              m_takeProfit, m_stopLoss),
          0, sourceSecurity, delayMeasurement);
    } catch (const CommunicationError &ex) {
      m_self.GetLog().Debug("Communication error at signal handling: \"%1%\".",
                            ex.what());
    }
  }
};

TrendRepeatingStrategy::TrendRepeatingStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf)
    : Base(context,
           "{C9C4282A-C620-45DA-9071-A6F9E5224BE9}",
           "PingPong",
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
  Verify(m_pimpl->m_securities
             .emplace(std::make_pair(
                 &security, boost::make_shared<Subscribtion>(
                                m_pimpl->m_fastMaSize, m_pimpl->m_slowMaSize)))
             .second);
  Base::OnSecurityStart(security, request);
}

void TrendRepeatingStrategy::OnLevel1Update(
    Security &security, const Milestones &delayMeasurement) {
  if (m_pimpl->m_isStopped) {
    return;
  }

  const auto &securityIt = m_pimpl->m_securities.find(&security);
  Assert(securityIt != m_pimpl->m_securities.cend());
  if (securityIt == m_pimpl->m_securities.cend()) {
    return;
  }
  Subscribtion &subscribtion = *securityIt->second;

  const auto &bid = security.GetBidPriceValue();
  const auto &ask = security.GetAskPriceValue();
  const auto &spread = ask - bid;
  const auto &statValue = bid + (spread / 2);

  subscribtion.marketDataBuffer.push_back(statValue);

  Ma &fastMa = subscribtion.fastMa;
  Ma &slowMa = subscribtion.slowMa;
  (*fastMa.stat)(statValue);
  (*slowMa.stat)(statValue);
  if (subscribtion.isEnabled && (!slowMa.isReported || !fastMa.isReported)) {
    if (slowMa.IsFilled() && slowMa.IsFilled()) {
      RaiseEvent(
          "Both " +
          GetTradingSystem(security.GetSource().GetIndex()).GetInstanceName() +
          " MAs are filled, starting signal checks...");
      slowMa.isReported = fastMa.isReported = true;
    }
    if (!slowMa.isReported && slowMa.IsFilled()) {
      RaiseEvent(
          GetTradingSystem(security.GetSource().GetIndex()).GetInstanceName() +
          " slow MA is filled, waiting for fast MA filling...");
      slowMa.isReported = true;
    }
    if (!fastMa.isReported && fastMa.IsFilled()) {
      RaiseEvent(
          GetTradingSystem(security.GetSource().GetIndex()).GetInstanceName() +
          " fast MA is filled, waiting for slow MA filling...");
      fastMa.isReported = true;
    }
  }

  m_pimpl->CheckSignal(security, subscribtion, delayMeasurement);
}

void TrendRepeatingStrategy::OnPositionUpdate(Position &position) {
  if (m_pimpl->m_isStopped) {
    return;
  }
  try {
    m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Debug("Communication error at position update handling: \"%1%\".",
                   ex.what());
  }
}

void TrendRepeatingStrategy::OnPostionsCloseRequest() {
  if (m_pimpl->m_isStopped) {
    return;
  }
  m_pimpl->m_controller.OnPostionsCloseRequest(*this);
}

bool TrendRepeatingStrategy::OnBlocked(const std::string *reason) noexcept {
  if (m_pimpl->m_isStopped) {
    return Base::OnBlocked(reason);
  }
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
  if (m_pimpl->m_controller.IsOpeningEnabled() == isEnabled) {
    return;
  }
  m_pimpl->m_controller.EnableOpening(isEnabled);
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
  return m_pimpl->m_controller.IsOpeningEnabled();
}

void TrendRepeatingStrategy::EnableActivePositionsControl(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_controller.IsClosingEnabled() == isEnabled) {
    return;
  }
  if (!isEnabled && m_pimpl->m_controller.IsOpeningEnabled()) {
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
  return m_pimpl->m_controller.IsClosingEnabled();
}

void TrendRepeatingStrategy::SetNumberOfFastMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(
      [this](Subscribtion &subscribtion) -> Ma & {
        return subscribtion.fastMa;
      },
      m_pimpl->m_fastMaSize, numberOfPeriods);
}
size_t TrendRepeatingStrategy::GetNumberOfFastMaPeriods() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_fastMaSize;
}
void TrendRepeatingStrategy::SetNumberOfSlowMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(
      [this](Subscribtion &subscribtion) -> Ma & {
        return subscribtion.slowMa;
      },
      m_pimpl->m_slowMaSize, numberOfPeriods);
}
size_t TrendRepeatingStrategy::GetNumberOfSlowMaPeriods() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_slowMaSize;
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
  if (m_pimpl->m_takeProfit->GetProfitShareToActivate() == takeProfit) {
    return;
  }
  GetTradingLog().Write("take profit: %1% -> %2%", [&](TradingRecord &record) {
    record % m_pimpl->m_takeProfit->GetProfitShareToActivate()  // 1
        % takeProfit;                                           // 2
  });
  *m_pimpl->m_takeProfit = TakeProfitShare::Params{
      takeProfit, m_pimpl->m_takeProfit->GetTrailingShareToClose()};
}
Double TrendRepeatingStrategy::GetTakeProfit() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_takeProfit->GetProfitShareToActivate();
}
void TrendRepeatingStrategy::SetTakeProfitTrailing(const Double &trailing) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_takeProfit->GetTrailingShareToClose() == trailing) {
    return;
  }
  GetTradingLog().Write(
      "take profit trailing: %1%-> %2%", [&](TradingRecord &record) {
        record % m_pimpl->m_takeProfit->GetTrailingShareToClose()  // 1
            % trailing;                                            // 2
      });
  *m_pimpl->m_takeProfit = TakeProfitShare::Params{
      m_pimpl->m_takeProfit->GetProfitShareToActivate(), trailing};
}
Double TrendRepeatingStrategy::GetTakeProfitTrailing() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_takeProfit->GetTrailingShareToClose();
}

sig::scoped_connection TrendRepeatingStrategy::SubscribeToEvents(
    const boost::function<void(const std::string &)> &slot) {
  return m_pimpl->m_eventsSignal.connect(slot);
}

sig::scoped_connection TrendRepeatingStrategy::SubscribeToBlocking(
    const boost::function<void(const std::string *)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

const tl::Trend &TrendRepeatingStrategy::GetTrend(
    const Security &security) const {
  const auto &securityIt = m_pimpl->m_securities.find(&security);
  Assert(securityIt != m_pimpl->m_securities.cend());
  if (securityIt == m_pimpl->m_securities.cend()) {
    throw LogicError("Request trend for unknown security");
  }
  return securityIt->second->trend;
}

void TrendRepeatingStrategy::RaiseEvent(const std::string &message) {
  m_pimpl->m_eventsSignal(message);
}

void TrendRepeatingStrategy::EnableTradingSystem(size_t tradingSystemIndex,
                                                 bool isEnabled) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() != tradingSystemIndex) {
      continue;
    }
    Subscribtion &subscribtion = *security.second;
    if (subscribtion.isEnabled == isEnabled) {
      continue;
    }
    subscribtion.isEnabled = isEnabled;
    subscribtion.fastMa.isReported = subscribtion.slowMa.isReported = false;
    GetTradingLog().Write("%1% %2%", [&](TradingRecord &record) {
      record % *security.first                           // 1
          % (security.second ? "enabled" : "disabled");  // 2
    });
  }
}

boost::tribool TrendRepeatingStrategy::IsTradingSystemEnabled(
    size_t tradingSystemIndex) const {
  for (const auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() == tradingSystemIndex) {
      return security.second->isEnabled;
    }
  }
  return boost::indeterminate;
}

void TrendRepeatingStrategy::Stop() noexcept {
  m_pimpl->m_isStopped = true;
  try {
    EnableTrading(false);
    EnableActivePositionsControl(false);
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
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
