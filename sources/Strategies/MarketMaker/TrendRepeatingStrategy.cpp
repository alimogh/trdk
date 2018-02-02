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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace Lib::TimeMeasurement;
using namespace trdk::Strategies::MarketMaker;

namespace accs = boost::accumulators;
namespace ma = trdk::Lib::Accumulators::MovingAverage;

namespace {

struct Ma {
  size_t numberOfPeriods;
  std::unique_ptr<ma::Exponential> ma;

  explicit Ma(size_t initialNumberOfPeriods)
      : numberOfPeriods(initialNumberOfPeriods) {
    Init();
  }

  void Init() {
    ma = boost::make_unique<ma::Exponential>(
        accs::tag::rolling_window::window_size = numberOfPeriods);
  }
};
}  // namespace

class TrendRepeatingStrategy::Implementation : private boost::noncopyable {
 public:
  TrendRepeatingStrategy &m_self;

  std::unique_ptr<ma::Exponential> m_fastMa;
  std::unique_ptr<ma::Exponential> m_slowMa;

  boost::shared_ptr<TakeProfitShare::Params> m_takeProfit;
  boost::shared_ptr<StopLossShare::Params> m_stopLoss;

  boost::circular_buffer<Price> m_marketDataBuffer;

 public:
  explicit Implementation(TrendRepeatingStrategy &self)
      : m_self(self),
        m_fastMa(boost::make_unique<ma::Exponential>(
            accs::tag::rolling_window::window_size = 12)),
        m_slowMa(boost::make_unique<ma::Exponential>(
            accs::tag::rolling_window::window_size = 26)),
        m_takeProfit(boost::make_shared<TakeProfitShare::Params>(
            TakeProfitShare::Params{.5 / 100, 0})),
        m_stopLoss(boost::make_shared<StopLossShare::Params>(
            StopLossShare::Params{5 / 100})),
        m_marketDataBuffer(std::max(accs::rolling_count(*m_fastMa),
                                    accs::rolling_count(*m_slowMa))) {}

  void SetNumberOfMaPeriods(std::unique_ptr<ma::Exponential> &ma,
                            size_t newNumberOfPeriods) {
    const auto lock = m_self.LockForOtherThreads();
    if (accs::rolling_count(*ma) == newNumberOfPeriods) {
      return;
    }
    ma.reset(new ma::Exponential(accs::tag::rolling_window::window_size =
                                     newNumberOfPeriods));
    for (auto it = m_marketDataBuffer.size() <= newNumberOfPeriods
                       ? m_marketDataBuffer.begin()
                       : m_marketDataBuffer.end() - newNumberOfPeriods;
         it != m_marketDataBuffer.end(); ++it) {
      (*ma)(*it);
    }
    if (m_marketDataBuffer.capacity() < newNumberOfPeriods) {
      m_marketDataBuffer.set_capacity(newNumberOfPeriods);
    }
  }
  size_t GetNumberOfMaPeriods(
      const std::unique_ptr<ma::Exponential> &ma) const {
    const auto lock = m_self.LockForOtherThreads();
    return accs::rolling_count(*ma);
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

void TrendRepeatingStrategy::OnSecurityStart(Security &, Security::Request &) {}

void TrendRepeatingStrategy::OnLevel1Update(Security &security,
                                            const Milestones &) {
  const auto &bid = security.GetBidPriceValue();
  const auto &ask = security.GetAskPriceValue();
  const auto &spread = ask - bid;
  const auto &statValue = bid + (spread / 2);
  m_pimpl->m_marketDataBuffer.push_back(statValue);
  (*m_pimpl->m_fastMa)(statValue);
  (*m_pimpl->m_slowMa)(statValue);
}

void TrendRepeatingStrategy::OnPositionUpdate(Position &) {}
void TrendRepeatingStrategy::OnPostionsCloseRequest() {}

bool TrendRepeatingStrategy::OnBlocked(const std::string *reason) noexcept {
  return Base::OnBlocked(reason);
}

void TrendRepeatingStrategy::SetPositionSize(const Qty &) {}

void TrendRepeatingStrategy::EnableTrading(bool) {}
void TrendRepeatingStrategy::EnableActivePositionsControl(bool) {}

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
  *m_pimpl->m_stopLoss = StopLossShare::Params{stopLoss};
}
Double TrendRepeatingStrategy::GetStopLoss() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_stopLoss->GetMaxLossShare();
}
void TrendRepeatingStrategy::SetTakeProfit(const Double &takeProfit) {
  const auto lock = LockForOtherThreads();
  *m_pimpl->m_takeProfit = TakeProfitShare::Params{takeProfit, 0};
}
Double TrendRepeatingStrategy::GetTakeProfit() const {
  const auto lock = LockForOtherThreads();
  return m_pimpl->m_takeProfit->GetMinProfitShareToActivate();
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateMaTrendRepeatingStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  throw boost::make_unique<TrendRepeatingStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
