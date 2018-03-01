/*******************************************************************************
 *   Created: 2018/02/21 01:09:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerStrategy.hpp"
#include "TakerController.hpp"
#include "TakerOperation.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace Lib::TimeMeasurement;
using namespace trdk::Strategies::MarketMaker;

namespace sig = boost::signals2;
namespace pt = boost::posix_time;

namespace {
struct Subscribtion {
  bool isEnabled;
};
}  // namespace

class TakerStrategy::Implementation : private boost::noncopyable {
 public:
  TakerStrategy &m_self;

  sig::signal<void()> m_completedSignal;
  sig::signal<void(const std::string &)> m_eventsSignal;
  sig::signal<void(const Volume &, const Volume &)> m_pnlUpdateSignal;
  sig::signal<void(const Volume &, const Volume &)> m_volumeUpdateSignal;
  sig::signal<void(const std::string *)> m_blockSignal;

  pt::ptime m_nextPeriodEnd;
  bool m_isStopped;

  size_t m_numerOfPeriods;
  Volume m_goalVolume;
  Volume m_maxLoss;
  size_t m_periodSizeMinutes;

  Price m_minPrice;
  Price m_maxPrice;

  Volume m_tradeMinVolume;
  Volume m_tradeMaxVolume;

  boost::unordered_map<Security *, Subscribtion> m_securities;
  boost::unordered_map<Security *, Subscribtion>::iterator m_nextSecurity;

  boost::mt19937 m_random;

  Volume m_completedVolume;
  Volume m_pnl;

  TakerController m_controller;

  boost::optional<Volume> m_nextVolume;

 public:
  explicit Implementation(TakerStrategy &self)
      : m_self(self),
        m_isStopped(false),
        m_numerOfPeriods(1),
        m_goalVolume(0.001),
        m_maxLoss(0.00001),
        m_periodSizeMinutes(15),
        m_minPrice(0),
        m_maxPrice(100000000),
        m_tradeMinVolume(0.0003),
        m_tradeMaxVolume(0.0003),
        m_nextSecurity(m_securities.end()) {}

  bool IsActive() const {
    return m_nextPeriodEnd != pt::not_a_date_time && !m_isStopped;
  }

  Volume GeneratePositionVolume() const {
    const auto min = std::min(m_tradeMinVolume, m_goalVolume);
    const auto max = std::min(m_tradeMaxVolume, m_goalVolume);
    if (min >= max) {
      return std::min(min, max);
    }
    boost::uniform_real<> range(min, max);
    boost::variate_generator<boost::mt19937, boost::uniform_real<>> generator(
        m_random, range);
    return generator();
  }

  void SetCompletedVolume(const Volume &newVolume) {
    if (m_completedVolume == newVolume) {
      return;
    }
    m_completedVolume = newVolume;
    m_volumeUpdateSignal(m_completedVolume, m_goalVolume);
  }
  void AddCompletedVolume(const Volume &volume) {
    if (!volume) {
      return;
    }
    m_completedVolume += volume;
    m_volumeUpdateSignal(m_completedVolume, m_goalVolume);
  }
  void SetPnl(const Volume &newPnl) {
    if (m_pnl == newPnl) {
      return;
    }
    m_pnl = newPnl;
    m_pnlUpdateSignal(m_pnl, m_maxLoss);
  }
  void AddPnl(const Volume &pnl) {
    if (!pnl) {
      return;
    }
    m_pnl += pnl;
    m_pnlUpdateSignal(m_pnl, m_maxLoss);
  }

  void StartNewIteration() {
    if (m_nextPeriodEnd == pt::not_a_date_time) {
      return;
    }

    m_self.GetTradingLog().Write("starting new iteration");

    for (auto &position : m_self.GetPositions()) {
      try {
        m_controller.ClosePosition(position, CLOSE_REASON_SCHEDULE);
      } catch (const CommunicationError &ex) {
        m_self.GetLog().Debug(
            "Communication error position closing scheduling: \"%1%\".",
            ex.what());
      }
    }

    if (m_nextVolume) {
      m_self.GetLog().Warn(
          "The previous iteration was not executed. Number of active "
          "positions: %1%.",
          m_self.GetPositions().GetSize());
    }

    m_nextVolume = GeneratePositionVolume();
    if (!m_nextVolume) {
      {
        const char *message = "No trade volume set.";
        m_self.RaiseEvent(message);
        m_self.GetLog().Error(message);
      }
      Complete();
      return;
    }

    for (;;) {
      const auto &now = m_self.GetContext().GetCurrentTime();
      if (now >= m_nextPeriodEnd) {
        if (Complete()) {
          return;
        } else {
          continue;
        }
      }
      const auto numberOfIteration =
          std::max(1, static_cast<int>(m_goalVolume / *m_nextVolume));
      const auto delay = (m_nextPeriodEnd - now) / numberOfIteration;
      m_self.GetTradingLog().Write(
          "scheduling new iteration after %1% at %2% (next volume: %3%, number "
          "of iteration: %4%, period end: %5%)",
          [&](TradingRecord &record) {
            record % delay                                        // 1
                % (m_self.GetContext().GetCurrentTime() + delay)  // 2
                % *m_nextVolume                                   // 3
                % numberOfIteration                               // 4
                % m_nextPeriodEnd;                                // 5
          });
      m_self.Schedule(delay, [this]() { StartNewIteration(); });
      break;
    }

    if (!m_self.GetPositions().IsEmpty()) {
      return;
    }

    StartNewOperation();
  }

  void StartNewOperation() {
    if (!m_nextVolume) {
      return;
    } else if (m_self.GetContext().GetCurrentTime() >= m_nextPeriodEnd) {
      Complete();
      return;
    }

    auto *const security = ChooseSecurityForNewOperation();
    if (!security) {
      return;
    }

    const bool isLong = true;

    const auto &price =
        isLong ? security->GetAskPriceValue() : security->GetBidPriceValue();
    if (price.IsNan() || !price) {
      return;
    }
    const auto qty = (*m_nextVolume / 2) / price;
    try {
      m_controller.OnSignal(
          boost::make_shared<TakerOperation>(m_self, isLong, qty), 0, *security,
          Milestones());
      m_nextVolume = boost::none;
    } catch (const CommunicationError &ex) {
      m_self.GetLog().Debug("Communication error at signal handling: \"%1%\".",
                            ex.what());
      m_self.Schedule(pt::seconds(15), [this]() { StartNewOperation(); });
    }
  }

  Security *ChooseSecurityForNewOperation() {
    Assert(m_securities.cbegin() != m_securities.cend());
    const auto end = m_nextSecurity;
    do {
      if (m_nextSecurity == m_securities.cend()) {
        m_nextSecurity = m_securities.begin();
      } else {
        if (m_nextSecurity->second.isEnabled) {
          return m_nextSecurity->first;
        }
        ++m_nextSecurity;
      }
    } while (m_nextSecurity != end);
    return nullptr;
  }

  bool Complete() {
    if (m_self.GetPositions().IsEmpty()) {
      m_self.RaiseEvent("Trading period is completed.");
      m_nextPeriodEnd = pt::not_a_date_time;
      m_completedSignal();
    }
    return true;
  }
};

TakerStrategy::TakerStrategy(Context &context,
                             const std::string &instanceName,
                             const IniSectionRef &conf)
    : Base(context,
           "{24895049-4058-4F3B-A4E8-AC6FE5549D4D}",
           "MarketMakerTaker",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

TakerStrategy::~TakerStrategy() = default;

void TakerStrategy::OnSecurityStart(Security &security,
                                    Security::Request &request) {
  if (!m_pimpl->m_securities.empty() &&
      m_pimpl->m_securities.begin()->first->GetSymbol() !=
          security.GetSymbol()) {
    throw Exception("Strategy works only with one symbol, but more is set");
  }
  Verify(m_pimpl->m_securities.emplace(&security, Subscribtion{false}).second);
  m_pimpl->m_nextSecurity = m_pimpl->m_securities.end();
  Base::OnSecurityStart(security, request);
}

void TakerStrategy::OnLevel1Update(Security &security,
                                   const Milestones &delayMeasurement) {
  if (m_pimpl->m_isStopped) {
    return;
  }
  security;
  delayMeasurement;
}

void TakerStrategy::OnPositionUpdate(Position &position) {
  if (m_pimpl->m_isStopped) {
    return;
  }

  if (position.IsCompleted()) {
    if (position.GetCloseReason() == CLOSE_REASON_NONE) {
      m_pimpl->AddCompletedVolume(position.GetOpenedVolume());
    }

    m_pimpl->AddCompletedVolume(position.GetClosedVolume());
    for (const auto &pnl : position.GetOperation()->GetPnl().GetData()) {
      if (pnl.first == position.GetSecurity().GetSymbol().GetQuoteSymbol()) {
        GetLog().Error("Wrong currency in P&L: \"%1%\" = %2%.",
                       pnl.first,    // 1
                       pnl.second);  // 2
        AssertEq(pnl.first,
                 position.GetSecurity().GetSymbol().GetQuoteSymbol());
      }
      m_pimpl->AddPnl(pnl.second);
    }

    m_pimpl->StartNewOperation();

    return;
  }

  const auto prevCloseReason = position.GetCloseReason();

  try {
    m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Debug("Communication error at position update handling: \"%1%\".",
                   ex.what());
  }

  if (prevCloseReason == CLOSE_REASON_NONE &&
      prevCloseReason != position.GetCloseReason()) {
    m_pimpl->AddCompletedVolume(position.GetOpenedVolume());
  }
}

void TakerStrategy::OnPostionsCloseRequest() {
  if (m_pimpl->m_isStopped) {
    return;
  }
  m_pimpl->m_controller.OnPostionsCloseRequest(*this);
}

bool TakerStrategy::OnBlocked(const std::string *reason) noexcept {
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

void TakerStrategy::EnableTrading(bool isEnabled) {
  const auto lock = LockForOtherThreads();

  if ((m_pimpl->m_nextPeriodEnd != pt::not_a_date_time) == isEnabled ||
      m_pimpl->m_isStopped) {
    return;
  }

  if (!isEnabled) {
    m_pimpl->m_nextPeriodEnd = pt::not_a_date_time;
    RaiseEvent("Trading disabled.");
    return;
  }

  if (m_pimpl->m_securities.empty()) {
    RaiseEvent(
        "Failed to enable trading as no one selected exchange supports "
        "specified symbol.");
    return;
  }

  m_pimpl->m_nextPeriodEnd =
      GetContext().GetCurrentTime() +
      pt::minutes(static_cast<long>(m_pimpl->m_periodSizeMinutes));
  m_pimpl->SetCompletedVolume(0);
  m_pimpl->SetPnl(0);
  m_pimpl->StartNewIteration();

  {
    boost::format message(
        "Trading enabled with period %1% minutes (ends at %2%).");
    message % m_pimpl->m_periodSizeMinutes  // 1
        % m_pimpl->m_nextPeriodEnd;         // 2
    RaiseEvent(message.str());
  }
}
bool TakerStrategy::IsTradingEnabled() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_nextPeriodEnd != pt::not_a_date_time;
}

sig::scoped_connection TakerStrategy::SubscribeToCompleted(
    const boost::function<void()> &slot) {
  return m_pimpl->m_completedSignal.connect(slot);
}

sig::scoped_connection TakerStrategy::SubscribeToEvents(
    const boost::function<void(const std::string &)> &slot) {
  return m_pimpl->m_eventsSignal.connect(slot);
}

sig::scoped_connection TakerStrategy::SubscribeToBlocking(
    const boost::function<void(const std::string *)> &slot) {
  return m_pimpl->m_blockSignal.connect(slot);
}

sig::scoped_connection TakerStrategy::SubscribeToVolume(
    const boost::function<void(const Volume &, const Volume &)> &slot) {
  return m_pimpl->m_volumeUpdateSignal.connect(slot);
}

sig::scoped_connection TakerStrategy::SubscribeToPnl(
    const boost::function<void(const Volume &, const Volume &)> &slot) {
  return m_pimpl->m_pnlUpdateSignal.connect(slot);
}

void TakerStrategy::RaiseEvent(const std::string &message) {
  GetTradingLog().Write("event: \"%1%\"", [&message](TradingRecord &record) {
    record % message;
  });
  m_pimpl->m_eventsSignal(message);
}

void TakerStrategy::EnableTradingSystem(size_t tradingSystemIndex,
                                        bool isEnabled) {
  const auto lock = LockForOtherThreads();
  for (auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() != tradingSystemIndex) {
      continue;
    }
    Subscribtion &subscribtion = security.second;
    if (subscribtion.isEnabled == isEnabled) {
      continue;
    }
    subscribtion.isEnabled = isEnabled;
    GetTradingLog().Write("%1% %2%", [&](TradingRecord &record) {
      record % *security.first                                     // 1
          % (security.second.isEnabled ? "enabled" : "disabled");  // 2
    });
  }
}

boost::tribool TakerStrategy::IsTradingSystemEnabled(
    size_t tradingSystemIndex) const {
  for (const auto &security : m_pimpl->m_securities) {
    if (security.first->GetSource().GetIndex() == tradingSystemIndex) {
      return security.second.isEnabled;
    }
  }
  return boost::indeterminate;
}

void TakerStrategy::Stop() noexcept {
  m_pimpl->m_isStopped = true;
  try {
    EnableTrading(false);
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void TakerStrategy::SetNumerOfPeriods(size_t value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_numerOfPeriods = value;
}
size_t TakerStrategy::GetNumerOfPeriods() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_numerOfPeriods;
}

void TakerStrategy::SetGoalVolume(const Volume &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_goalVolume = value;
  m_pimpl->m_volumeUpdateSignal(m_pimpl->m_completedVolume,
                                m_pimpl->m_goalVolume);
}
Volume TakerStrategy::GetGoalVolume() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_goalVolume;
}

void TakerStrategy::SetMaxLoss(const Volume &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_maxLoss = value;
  m_pimpl->m_pnlUpdateSignal(m_pimpl->m_pnl, m_pimpl->m_maxLoss);
}
Volume TakerStrategy::GetMaxLoss() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_maxLoss;
}

void TakerStrategy::SetPeriodSize(size_t numberOfMinutes) {
  const auto lock = LockForOtherThreads();
  std::swap(numberOfMinutes, m_pimpl->m_periodSizeMinutes);
  if (m_pimpl->m_nextPeriodEnd == pt::not_a_date_time) {
    return;
  }
  m_pimpl->m_nextPeriodEnd =
      m_pimpl->m_nextPeriodEnd -
      pt::minutes(static_cast<long>(numberOfMinutes)) +
      pt::minutes(static_cast<long>(m_pimpl->m_periodSizeMinutes));

  {
    boost::format message(
        "Trading period changed - %1% minutes (ends at %2%).");
    message % m_pimpl->m_periodSizeMinutes  // 1
        % m_pimpl->m_nextPeriodEnd;         // 2
    RaiseEvent(message.str());
  }

  if (GetContext().GetCurrentTime() >= m_pimpl->m_nextPeriodEnd) {
    m_pimpl->Complete();
  }
}
size_t TakerStrategy::GetPeriodSize() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_periodSizeMinutes;
}

void TakerStrategy::SetMinPrice(const Price &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_minPrice = value;
}
Price TakerStrategy::GetMinPrice() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_minPrice;
}

void TakerStrategy::SetMaxPrice(const Price &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_maxPrice = value;
}
Price TakerStrategy::GetMaxPrice() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_maxPrice;
}

void TakerStrategy::SetTradeMinVolume(const Volume &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_tradeMinVolume = value;
}
Volume TakerStrategy::GetTradeMinVolume() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_tradeMinVolume;
}

void TakerStrategy::SetTradeMaxVolume(const Volume &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_tradeMaxVolume = value;
}
Volume TakerStrategy::GetTradeMaxVolume() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_tradeMaxVolume;
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf) {
  return boost::make_unique<TakerStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
