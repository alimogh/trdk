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
using namespace Lib::TimeMeasurement;
using namespace trdk::Strategies::MarketMaker;

// namespace ma = trdk::Lib::Accumulators::MovingAverage;

class TrendRepeatingStrategy::Implementation : private boost::noncopyable {
  // std::unique_ptr<ma::Exponential> m_acc;
};

TrendRepeatingStrategy::TrendRepeatingStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf)
    : Base(context,
           "{C9C4282A-C620-45DA-9071-A6F9E5224BE9}",
           "MaTrendRepeatingMarketMaker",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>()) {}

TrendRepeatingStrategy::~TrendRepeatingStrategy() = default;

void TrendRepeatingStrategy::OnSecurityStart(Security &, Security::Request &) {}

void TrendRepeatingStrategy::OnLevel1Update(Security &, const Milestones &) {}

void TrendRepeatingStrategy::OnPositionUpdate(Position &) {}
void TrendRepeatingStrategy::OnPostionsCloseRequest() {}

bool TrendRepeatingStrategy::OnBlocked(const std::string *reason) noexcept {
  return Base::OnBlocked(reason);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<trdk::Strategy> CreateMaTrendRepeatingStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  throw boost::make_unique<TrendRepeatingStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
