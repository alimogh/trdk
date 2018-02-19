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

  boost::unordered_map<const Security *, Subscribtion> m_securities;

  boost::mt19937 m_random;
  boost::variate_generator<boost::mt19937, boost::uniform_real<>>
      m_positionVolumeGenerator;

  Volume m_completedVolume;
  Volume m_pnl;

 public:
  explicit Implementation(TakerStrategy &self)
      : m_self(self),
        m_isStopped(false),
        m_numerOfPeriods(1),
        m_goalVolume(1),
        m_maxLoss(1),
        m_periodSizeMinutes(15),
        m_minPrice(0),
        m_maxPrice(100000000),
        m_tradeMinVolume(0),
        m_tradeMaxVolume(100000000),
        m_positionVolumeGenerator(CreatePositionVolumeGenerator()) {}

  bool IsActive() const {
    return m_nextPeriodEnd != pt::not_a_date_time && !m_isStopped;
  }

  boost::variate_generator<boost::mt19937, boost::uniform_real<>>
  CreatePositionVolumeGenerator() const {
    boost::uniform_real<> range(m_tradeMinVolume, m_tradeMaxVolume);
    boost::variate_generator<boost::mt19937, boost::uniform_real<>> result(
        m_random, range);
    return result;
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
  position;
  try {
    // m_pimpl->m_controller.OnPositionUpdate(position);
  } catch (const CommunicationError &ex) {
    GetLog().Debug("Communication error at position update handling: \"%1%\".",
                   ex.what());
  }
}

void TakerStrategy::OnPostionsCloseRequest() {
  if (m_pimpl->m_isStopped) {
    return;
  }
  // m_pimpl->m_controller.OnPostionsCloseRequest(*this);
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
  m_pimpl->m_nextPeriodEnd =
      isEnabled ? GetContext().GetCurrentTime() + pt::minutes(isEnabled)
                : pt::not_a_date_time;
  if (isEnabled) {
    if (m_pimpl->m_securities.empty()) {
      RaiseEvent(
          "Failed to enable trading as no one selected exchange supports "
          "specified symbol.");
      return;
    }
  }
  GetTradingLog().Write("%1% trading", [isEnabled](TradingRecord &record) {
    record % (isEnabled ? "enabled" : "disabled");
  });
  if (!isEnabled) {
    return;
  }
  m_pimpl->SetCompletedVolume(0);
  m_pimpl->SetPnl(0);
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
  m_pimpl->m_periodSizeMinutes = numberOfMinutes;
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
  m_pimpl->m_positionVolumeGenerator = m_pimpl->CreatePositionVolumeGenerator();
}
Volume TakerStrategy::GetTradeMinVolume() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_tradeMinVolume;
}

void TakerStrategy::SetTradeMaxVolume(const Volume &value) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_tradeMaxVolume = value;
  m_pimpl->m_positionVolumeGenerator = m_pimpl->CreatePositionVolumeGenerator();
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
