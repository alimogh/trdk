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
#include "Strategy.hpp"
#include "Controller.hpp"
#include "Operation.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace Lib::TimeMeasurement;
using namespace trdk::Strategies::PingPong;

namespace accs = boost::accumulators;
namespace ma = trdk::Lib::Accumulators::MovingAverage;
namespace sig = boost::signals2;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;
namespace tl = trdk::TradingLib;
namespace pp = trdk::Strategies::PingPong;

using tl::StopLossShare;
using tl::TakeProfitShare;

namespace {
struct Subscribtion {
  bool isEnabled;
  boost::circular_buffer<Price> marketDataBuffer;
  pp::Strategy::Indicators indicators;
  pp::Strategy::Trends trends;

  explicit Subscribtion(size_t fastMaSize,
                        size_t slowMaSize,
                        size_t numberOfRsiPeriods,
                        const Double &overboughtLevel,
                        const Double &oversoldLevel,
                        const pp::Strategy::IndicatorsToggles &toggles)
      : isEnabled(true),
        marketDataBuffer(
            std::max(fastMaSize, std::max(slowMaSize, numberOfRsiPeriods))),
        indicators(fastMaSize,
                   slowMaSize,
                   numberOfRsiPeriods,
                   overboughtLevel,
                   oversoldLevel),
        trends(toggles) {}
};
}  // namespace

pp::Strategy::Indicators::Ma::Ma(const char *name, size_t numberOfPeriods)
    : name(name) {
  Reset(numberOfPeriods);
}
void pp::Strategy::Indicators::Ma::Reset(size_t newNumberOfPeriods) {
  stat.reset(new ma::Exponential(accs::tag::rolling_window::window_size =
                                     newNumberOfPeriods));
  numberOfPeriods = newNumberOfPeriods;
}
pp::Strategy::Indicators::Indicators(size_t fastMaSize,
                                     size_t slowMaSize,
                                     size_t numberOfRsiPeriods,
                                     const Double &overboughtLevel,
                                     const Double &oversoldLevel)
    : fastMa("fast", fastMaSize),
      slowMa("slow", slowMaSize),
      rsi(Rsi{RelativeStrengthIndex(numberOfRsiPeriods), overboughtLevel,
              oversoldLevel}) {}
bool pp::Strategy::Indicators::Update(const Price &value) {
  (*fastMa.stat)(value);
  (*slowMa.stat)(value);
  rsi.stat.Append(value);
  return accs::rolling_count(*slowMa.stat) >= slowMa.numberOfPeriods &&
         accs::rolling_count(*fastMa.stat) >= fastMa.numberOfPeriods &&
         rsi.stat.IsFull();
}

pp::Strategy::Trend::Trend(const IndicatorsToggles::Toggles &toggles)
    : m_toggles(toggles) {}
bool pp::Strategy::Trend::Update(const ma::Exponential &slowMa,
                                 const ma::Exponential &fastMa) {
  return SetIsRising(accs::ema(slowMa) < accs::ema(fastMa));
}
bool pp::Strategy::Trend::Update(const Indicators::Rsi &rsi) {
  return rsi.stat.GetLastValue() > rsi.overboughtLevel
             ? SetIsRising(false)
             : rsi.stat.GetLastValue() < rsi.oversoldLevel ? SetIsRising(true)
                                                           : Reset();
}

const pp::Strategy::IndicatorsToggles::Toggles &
pp::Strategy::Trend::GetIndicatorsToggles() const {
  return m_toggles;
}

pp::Strategy::Trends::Trends(const IndicatorsToggles &toggles)
    : ma(toggles.ma), rsi(toggles.rsi) {}

bool pp::Strategy::Trends::Update(const Indicators &indicators) {
  bool result = false;
  if (ma.Update(*indicators.slowMa.stat, *indicators.fastMa.stat)) {
    if (ma.GetIndicatorsToggles().isOpeningSignalConfirmationEnabled) {
      result = true;
    }
  }
  if (rsi.Update(indicators.rsi)) {
    if (rsi.GetIndicatorsToggles().isOpeningSignalConfirmationEnabled) {
      result = true;
    }
  }
  return result;
}
namespace {
template <bool isOpen, typename Indicator>
bool IsRising(const Indicator &indicator, boost::optional<bool> &isRising) {
  if ((isOpen &&
       !indicator.GetIndicatorsToggles().isOpeningSignalConfirmationEnabled) ||
      (!isOpen &&
       !indicator.GetIndicatorsToggles().isClosingSignalConfirmationEnabled)) {
    return true;
  }
  if (!isRising) {
    isRising = indicator.IsRising();
  } else if (*isRising != indicator.IsRising()) {
    return false;
  }
  return true;
}
}  // namespace
bool pp::Strategy::Trends::IsRisingToOpen() const {
  boost::optional<bool> result;
  return IsRising<true>(ma, result) && IsRising<true>(rsi, result) && result &&
         *result;
}
bool pp::Strategy::Trends::HasCloseSignal(bool isLong) const {
  boost::optional<bool> isRising;
  if (IsRising<false>(ma, isRising) || IsRising<false>(rsi, isRising) ||
      !isRising) {
    return false;
  }
  return *isRising != isLong;
}

const ids::uuid pp::Strategy::typeId =
    ids::string_generator()("{C9C4282A-C620-45DA-9071-A6F9E5224BE9}");

class pp::Strategy::Implementation : private boost::noncopyable {
 public:
  pp::Strategy &m_self;

  Qty m_positionSize;

  IndicatorsToggles m_indicatorsToggles;

  size_t m_fastMaSize;
  size_t m_slowMaSize;

  size_t m_numberOfRsiPeriods;
  Double m_rsiOverboughtLevel;
  Double m_rsiOversoldLevel;

  boost::shared_ptr<TakeProfitShare::Params> m_takeProfit;
  boost::shared_ptr<StopLossShare::Params> m_stopLoss;

  sig::signal<void(const std::string &)> m_eventsSignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  Controller m_controller;

  boost::unordered_map<const Security *, boost::shared_ptr<Subscribtion>>
      m_securities;

  bool m_isStopped;

  // debug
  pt::time_duration m_frameSize;
  pt::ptime m_lastTime;

 public:
  explicit Implementation(Strategy &self, const IniSectionRef &conf)
      : m_self(self),
        m_indicatorsToggles(
            {{conf.ReadBoolKey("ma_opening_signal_confirmation_enabled"),
              conf.ReadBoolKey("ma_closing_signal_confirmation_enabled")},
             {conf.ReadBoolKey("rsi_opening_signal_confirmation_enabled"),
              conf.ReadBoolKey("rsi_closing_signal_confirmation_enabled")}}),
        m_positionSize(conf.ReadTypedKey<Qty>("position_size")),
        m_fastMaSize(conf.ReadTypedKey<size_t>("number_of_fast_ma_periods")),
        m_slowMaSize(conf.ReadTypedKey<size_t>("number_of_slow_ma_periods")),
        m_numberOfRsiPeriods(
            conf.ReadTypedKey<size_t>("number_of_rsi_periods")),
        m_rsiOverboughtLevel(conf.ReadTypedKey<Double>("rsi_overbought_level")),
        m_rsiOversoldLevel(conf.ReadTypedKey<Double>("rsi_oversold_level")),
        m_takeProfit(boost::make_shared<TakeProfitShare::Params>(
            conf.ReadTypedKey<Double>("profit_share_to_activate_take_profit") /
                100,
            conf.ReadTypedKey<Double>("take_profit_trailing_share_to_close") /
                100)),
        m_stopLoss(boost::make_shared<StopLossShare::Params>(
            conf.ReadTypedKey<Double>("max_loss_share") / 100)),
        m_controller(conf.ReadBoolKey("long_trading_enabled"),
                     conf.ReadBoolKey("short_trading_enabled"),
                     conf.ReadBoolKey("active_positions_control_enabled")),
        m_isStopped(false),
        m_frameSize(
            pt::seconds(conf.ReadTypedKey<long>("source_time_frame_size_sec"))),
        m_lastTime(m_self.GetContext().GetCurrentTime()) {}

  template <typename GetMa>
  void SetNumberOfMaPeriods(const GetMa &getMa,
                            size_t &currentNumberOfPeriods,
                            size_t newNumberOfPeriods) {
    const auto lock = m_self.LockForOtherThreads();
    for (auto &security : m_securities) {
      Subscribtion &subscribtion = *security.second;
      auto &ma = getMa(subscribtion);

      if (ma.numberOfPeriods == newNumberOfPeriods) {
        continue;
      }

      const auto prevNumberOfPeriods = ma.numberOfPeriods;
      const auto prevFilledSize = accs::rolling_count(*ma.stat);
      ma.Reset(newNumberOfPeriods);

      for (auto it =
               subscribtion.marketDataBuffer.size() <= newNumberOfPeriods
                   ? subscribtion.marketDataBuffer.begin()
                   : subscribtion.marketDataBuffer.end() - newNumberOfPeriods;
           it != subscribtion.marketDataBuffer.end(); ++it) {
        (*ma.stat)(*it);
      }

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

  void CheckSignal(Security &,
                   Subscribtion &subscribtion,
                   const Milestones &delayMeasurement) {
    if (!subscribtion.trends.Update(subscribtion.indicators)) {
      return;
    }
    if ((!m_controller.IsLongOpeningEnabled() &&
         !m_controller.IsShortOpeningEnabled()) ||
        !subscribtion.isEnabled) {
      return;
    }
    for (auto &security : m_self.GetSecurities()) {
      if (m_controller.HasPositions(m_self, security)) {
        continue;
      }
      try {
        m_controller.OnSignal(
            boost::make_shared<Operation>(m_self, m_positionSize,
                                          subscribtion.trends.IsRisingToOpen(),
                                          m_takeProfit, m_stopLoss),
            0, security, delayMeasurement);
      } catch (const CommunicationError &ex) {
        m_self.GetLog().Debug(
            "Communication error at signal handling: \"%1%\".", ex.what());
      }
    }
  }
};

pp::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf)
    : Base(context, typeId, "PingPong", instanceName, conf),
      m_pimpl(boost::make_unique<Implementation>(*this, conf)) {}

pp::Strategy::~Strategy() = default;

void pp::Strategy::OnSecurityStart(Security &security,
                                   Security::Request &request) {
  if (!m_pimpl->m_securities.empty() &&
      m_pimpl->m_securities.begin()->first->GetSymbol() !=
          security.GetSymbol()) {
    throw Exception("Strategy works only with one symbol, but more is set");
  }
  Verify(
      m_pimpl->m_securities
          .emplace(std::make_pair(
              &security,
              boost::make_shared<Subscribtion>(
                  m_pimpl->m_fastMaSize, m_pimpl->m_slowMaSize,
                  m_pimpl->m_numberOfRsiPeriods, m_pimpl->m_rsiOverboughtLevel,
                  m_pimpl->m_rsiOversoldLevel, m_pimpl->m_indicatorsToggles)))
          .second);
  Base::OnSecurityStart(security, request);
}

void pp::Strategy::OnLevel1Update(Security &security,
                                  const Milestones &delayMeasurement) {
  if (m_pimpl->m_isStopped) {
    return;
  }

  if (m_pimpl->m_lastTime + m_pimpl->m_frameSize >
      GetContext().GetCurrentTime()) {
    return;
  }
  m_pimpl->m_lastTime = GetContext().GetCurrentTime();

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
  if (subscribtion.indicators.Update(statValue)) {
    m_pimpl->CheckSignal(security, subscribtion, delayMeasurement);
  }
}

void pp::Strategy::OnPositionUpdate(Position &position) {
  if (m_pimpl->m_isStopped) {
    return;
  }
  try {
    m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Warn("Communication error at position update handling: \"%1%\".",
                  ex.what());
  }
}

void pp::Strategy::OnPostionsCloseRequest() {
  if (m_pimpl->m_isStopped) {
    return;
  }
  m_pimpl->m_controller.OnPostionsCloseRequest(*this);
}

bool pp::Strategy::OnBlocked(const std::string *reason) noexcept {
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

void pp::Strategy::SetPositionSize(const Qty &size) {
  const auto lock = LockForOtherThreads();
  GetTradingLog().Write("position size: %1% -> %2%",
                        [&](TradingRecord &record) {
                          record % m_pimpl->m_positionSize  // 1
                              % size;                       // 2
                        });
  m_pimpl->m_positionSize = size;
}
const Qty &pp::Strategy::GetPositionSize() const {
  return m_pimpl->m_positionSize;
}

void pp::Strategy::EnableLongTrading(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_controller.IsLongOpeningEnabled() == isEnabled) {
    return;
  }
  m_pimpl->m_controller.EnableLongOpening(isEnabled);
  if (isEnabled) {
    if (m_pimpl->m_securities.empty()) {
      RaiseEvent(
          "Failed to enable long trading as no one selected exchange supports "
          "specified symbol.");
      return;
    }
    m_pimpl->m_controller.EnableClosing(true);
  }
  GetTradingLog().Write("%1% long trading", [isEnabled](TradingRecord &record) {
    record % (isEnabled ? "enabled" : "disabled");
  });
}
void pp::Strategy::EnableShortTrading(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_controller.IsShortOpeningEnabled() == isEnabled) {
    return;
  }
  m_pimpl->m_controller.EnableShortOpening(isEnabled);
  if (isEnabled) {
    if (m_pimpl->m_securities.empty()) {
      RaiseEvent(
          "Failed to enable short trading as no one selected exchange supports "
          "specified symbol.");
      return;
    }
    m_pimpl->m_controller.EnableClosing(true);
  }
  GetTradingLog().Write("%1% short trading",
                        [isEnabled](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
}
bool pp::Strategy::IsLongTradingEnabled() const {
  return m_pimpl->m_controller.IsLongOpeningEnabled();
}
bool pp::Strategy::IsShortTradingEnabled() const {
  return m_pimpl->m_controller.IsShortOpeningEnabled();
}

void pp::Strategy::SetSourceTimeFrameSize(const pt::time_duration &frameSize) {
  GetTradingLog().Write(
      "time frame size: %1%",
      [&frameSize](TradingRecord &record) { record % frameSize; });
  m_pimpl->m_frameSize = frameSize;
}

const pt::time_duration &pp::Strategy::GetSourceTimeFrameSize() const {
  return m_pimpl->m_frameSize;
}

void pp::Strategy::EnableActivePositionsControl(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  if (m_pimpl->m_controller.IsClosingEnabled() == isEnabled) {
    return;
  }
  m_pimpl->m_controller.EnableClosing(isEnabled);
  GetTradingLog().Write("%1% position control",
                        [isEnabled](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
}
bool pp::Strategy::IsActivePositionsControlEnabled() const {
  return m_pimpl->m_controller.IsClosingEnabled();
}

bool pp::Strategy::IsMaOpeningSignalConfirmationEnabled() const {
  return m_pimpl->m_indicatorsToggles.ma.isOpeningSignalConfirmationEnabled;
}
bool pp::Strategy::IsMaClosingSignalConfirmationEnabled() const {
  return m_pimpl->m_indicatorsToggles.ma.isClosingSignalConfirmationEnabled;
}
void pp::Strategy::EnableMaOpeningSignalConfirmation(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  auto &toggle =
      m_pimpl->m_indicatorsToggles.ma.isOpeningSignalConfirmationEnabled;
  if (toggle == isEnabled) {
    return;
  }
  GetTradingLog().Write("%1% MA opening signal confirmation",
                        [&](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
  toggle = isEnabled;
}

void pp::Strategy::EnableMaClosingSignalConfirmation(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  auto &toggle =
      m_pimpl->m_indicatorsToggles.ma.isClosingSignalConfirmationEnabled;
  if (toggle == isEnabled) {
    return;
  }
  GetTradingLog().Write("%1% MA closing signal confirmation",
                        [&](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
  toggle = isEnabled;
}

void pp::Strategy::SetNumberOfFastMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(
      [this](Subscribtion &subscribtion) -> Indicators::Ma & {
        return subscribtion.indicators.fastMa;
      },
      m_pimpl->m_fastMaSize, numberOfPeriods);
}
size_t pp::Strategy::GetNumberOfFastMaPeriods() const {
  return m_pimpl->m_fastMaSize;
}
void pp::Strategy::SetNumberOfSlowMaPeriods(size_t numberOfPeriods) {
  m_pimpl->SetNumberOfMaPeriods(
      [this](Subscribtion &subscribtion) -> Indicators::Ma & {
        return subscribtion.indicators.slowMa;
      },
      m_pimpl->m_slowMaSize, numberOfPeriods);
}
size_t pp::Strategy::GetNumberOfSlowMaPeriods() const {
  return m_pimpl->m_slowMaSize;
}

bool pp::Strategy::IsRsiOpeningSignalConfirmationEnabled() const {
  return m_pimpl->m_indicatorsToggles.rsi.isOpeningSignalConfirmationEnabled;
}
bool pp::Strategy::IsRsiClosingSignalConfirmationEnabled() const {
  return m_pimpl->m_indicatorsToggles.rsi.isClosingSignalConfirmationEnabled;
}
void pp::Strategy::EnableRsiOpeningSignalConfirmation(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  auto &toggle =
      m_pimpl->m_indicatorsToggles.rsi.isOpeningSignalConfirmationEnabled;
  if (toggle == isEnabled) {
    return;
  }
  GetTradingLog().Write("%1% RSI opening signal confirmation",
                        [&](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
  toggle = isEnabled;
}
void pp::Strategy::EnableRsiClosingSignalConfirmation(bool isEnabled) {
  const auto lock = LockForOtherThreads();
  auto &toggle =
      m_pimpl->m_indicatorsToggles.rsi.isClosingSignalConfirmationEnabled;
  if (toggle == isEnabled) {
    return;
  }
  GetTradingLog().Write("%1% RSI closing signal confirmation",
                        [&](TradingRecord &record) {
                          record % (isEnabled ? "enabled" : "disabled");
                        });
  toggle = isEnabled;
}
size_t pp::Strategy::GetNumberOfRsiPeriods() const {
  return m_pimpl->m_numberOfRsiPeriods;
}
void pp::Strategy::SetNumberOfRsiPeriods(size_t newNumberOfPeriods) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    Subscribtion &subscribtion = *security.second;
    auto &rsi = subscribtion.indicators.rsi;
    const auto prevNumberOfPeriods = rsi.stat.GetNumberOfPeriods();
    if (prevNumberOfPeriods == newNumberOfPeriods) {
      continue;
    }

    rsi.stat = RelativeStrengthIndex(newNumberOfPeriods);

    for (auto it =
             subscribtion.marketDataBuffer.size() <= newNumberOfPeriods
                 ? subscribtion.marketDataBuffer.begin()
                 : subscribtion.marketDataBuffer.end() - newNumberOfPeriods;
         it != subscribtion.marketDataBuffer.end(); ++it) {
      rsi.stat.Append(*it);
    }

    GetTradingLog().Write("RSI size for \"%1%\": %2% -> %3%",
                          [&](TradingRecord &record) {
                            record % *security.first   // 1
                                % prevNumberOfPeriods  // 2
                                % newNumberOfPeriods;  // 3
                          });

    if (subscribtion.marketDataBuffer.capacity() < newNumberOfPeriods) {
      GetTradingLog().Write(
          "RSI buffer size: %1% -> %2%", [&](TradingRecord &record) {
            record % subscribtion.marketDataBuffer.capacity()  // 1
                % newNumberOfPeriods;                          // 2
          });
      subscribtion.marketDataBuffer.set_capacity(newNumberOfPeriods);
    }
  }
  m_pimpl->m_numberOfRsiPeriods = newNumberOfPeriods;
}

void pp::Strategy::SetStopLoss(const Double &stopLoss) {
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

const Double &pp::Strategy::GetRsiOverboughtLevel() const {
  return m_pimpl->m_rsiOverboughtLevel;
}
void pp::Strategy::SetRsiOverboughtLevel(const Double &value) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    Subscribtion &subscribtion = *security.second;
    auto &rsi = subscribtion.indicators.rsi;
    const auto overboughtLevel = rsi.overboughtLevel;
    if (overboughtLevel == value) {
      continue;
    }
    rsi.overboughtLevel = value;
    GetTradingLog().Write("RSI overbought level for \"%1%\": %2% -> %3%",
                          [&](TradingRecord &record) {
                            record % *security.first  // 1
                                % overboughtLevel     // 2
                                % value;              // 3
                          });
  }
  m_pimpl->m_rsiOverboughtLevel = value;
}
const Double &pp::Strategy::GetRsiOversoldLevel() const {
  return m_pimpl->m_rsiOversoldLevel;
}
void pp::Strategy::SetRsiOversoldLevel(const Double &value) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    Subscribtion &subscribtion = *security.second;
    auto &rsi = subscribtion.indicators.rsi;
    const auto oversoldLevel = rsi.oversoldLevel;
    if (oversoldLevel == value) {
      continue;
    }
    rsi.oversoldLevel = value;
    GetTradingLog().Write("RSI oversold level for \"%1%\": %2% -> %3%",
                          [&](TradingRecord &record) {
                            record % *security.first  // 1
                                % oversoldLevel       // 2
                                % value;              // 3
                          });
  }
  m_pimpl->m_rsiOversoldLevel = value;
}

const Double &pp::Strategy::GetStopLoss() const {
  return m_pimpl->m_stopLoss->GetMaxLossShare();
}
void pp::Strategy::SetTakeProfit(const Double &takeProfit) {
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
Double pp::Strategy::GetTakeProfit() const {
  return m_pimpl->m_takeProfit->GetProfitShareToActivate();
}
void pp::Strategy::SetTakeProfitTrailing(const Double &trailing) {
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
Double pp::Strategy::GetTakeProfitTrailing() const {
  return m_pimpl->m_takeProfit->GetTrailingShareToClose();
}

sig::scoped_connection pp::Strategy::SubscribeToEvents(
    const boost::function<void(const std::string &)> &slot) {
  return m_pimpl->m_eventsSignal.connect(slot);
}

sig::scoped_connection pp::Strategy::SubscribeToBlocking(
    const boost::function<void(const std::string *)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

const pp::Strategy::Trends &pp::Strategy::GetTrends(
    const Security &security) const {
  const auto &securityIt = m_pimpl->m_securities.find(&security);
  Assert(securityIt != m_pimpl->m_securities.cend());
  if (securityIt == m_pimpl->m_securities.cend()) {
    throw LogicError("Request trend list for unknown security");
  }
  return securityIt->second->trends;
}

void pp::Strategy::RaiseEvent(const std::string &message) {
  GetTradingLog().Write("event: \"%1%\"", [&message](TradingRecord &record) {
    record % message;
  });
  m_pimpl->m_eventsSignal(message);
}

void pp::Strategy::EnableTradingSystem(size_t tradingSystemIndex,
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
    GetTradingLog().Write("%1% %2%", [&](TradingRecord &record) {
      record % *security.first                                  // 1
          % (subscribtion.isEnabled ? "enabled" : "disabled");  // 2
    });
  }
}

boost::tribool pp::Strategy::IsTradingSystemEnabled(
    size_t tradingSystemIndex) const {
  for (const auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() == tradingSystemIndex) {
      return security.second->isEnabled;
    }
  }
  return boost::indeterminate;
}

void pp::Strategy::Stop() noexcept {
  m_pimpl->m_isStopped = true;
  try {
    EnableLongTrading(false);
    EnableShortTrading(false);
    EnableActivePositionsControl(false);
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf) {
  return boost::make_unique<pp::Strategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
