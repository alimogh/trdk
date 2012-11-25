/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "PositionReporter.hpp"
#include "Position.hpp"
#include "Settings.hpp"
#include "PositionBundle.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

Strategy::Notifier::Notifier() {
	//...//
}

Strategy::Notifier::~Notifier() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class Strategy::Implementation : private boost::noncopyable {

public:

	typedef SignalConnectionList<Position::StateUpdateConnection>
		StateUpdateConnections;

public:

	Strategy &m_strategy;

	boost::shared_ptr<PositionBandle> m_positions;
		
//	boost::shared_ptr<Notifier> m_notifier;
	StateUpdateConnections m_stateUpdateConnections;

	volatile int64_t m_isBlocked;

	PositionReporter *m_positionReporter;

public:

	explicit Implementation(
				Strategy &strategy)
			: m_strategy(strategy),
			m_isBlocked(false),
			m_positionReporter(nullptr) {
		//...//
	}

	~Implementation() {
		delete m_positionReporter;
	}

	bool CheckPositionsUnsafe(Notifier &notifier) {

		const auto now = boost::get_system_time();

		Assert(!m_isBlocked);
		
		// must be checked it security object
		Assert(
			m_strategy.GetSecurity()
			|| !m_strategy.GetSettings().ShouldWaitForMarketData());

		if (m_positions) {
			Assert(!m_positions->Get().empty());
			Assert(m_stateUpdateConnections.IsConnected());
			ReportClosedPositon(*m_positions);
			if (!m_positions->IsCompleted()) {
				if (m_positions->IsOk()) {
					if (	!m_strategy.GetSettings().IsReplayMode()
							&& m_strategy.GetSettings().GetCurrentTradeSessionEndime()
								<= now) {
						foreach (auto &p, m_positions->Get()) {
							if (p->IsCanceled()) {
								continue;
							}
							p->CancelAtMarketPrice(
								Position::CLOSE_TYPE_SCHEDULE);
						}
					} else {
						m_strategy.TryToClosePositions(*m_positions);
					}
				} else {
					Log::Warn(
						"Strategy \"%1%\" BLOCKED by dispatcher.",
						m_strategy);
					Interlocking::Exchange(m_isBlocked, true);
				}
				return false;
			}
			m_stateUpdateConnections.Diconnect();
			m_positions.reset();
		}

		Assert(!m_stateUpdateConnections.IsConnected());

		boost::shared_ptr<PositionBandle> positions
			= m_strategy.TryToOpenPositions();
		if (!positions || positions->Get().empty()) {
			return false;
		}

		StateUpdateConnections stateUpdateConnections;
		foreach (const auto &p, positions->Get()) {
			Assert(&p->GetSecurity() == &m_strategy.GetSecurity());
			m_strategy.ReportDecision(*p);
			stateUpdateConnections.InsertSafe(
				p->Subscribe(
					boost::bind(
						&Notifier::Signal,
						&notifier,
						m_strategy.shared_from_this())));
		}
		Assert(stateUpdateConnections.IsConnected());

		stateUpdateConnections.Swap(m_stateUpdateConnections);
		positions.swap(m_positions);
		return true;

	}

	void ReportClosedPositon(PositionBandle &positions) {
		Assert(!positions.Get().empty());
		foreach (auto &p, positions.Get()) {
			if (	p->IsOpened()
					&& !p->IsReported()
					&& (p->IsClosed() || p->IsError())) {
				m_strategy.GetPositionReporter().ReportClosedPositon(*p);
				p->MarkAsReported();
			}
		}
	}

};

//////////////////////////////////////////////////////////////////////////

Strategy::Strategy(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			boost::shared_ptr<const Settings> settings)
		: SecurityAlgo(tag, security, settings) {
	m_pimpl = new Implementation(*this);
}

Strategy::~Strategy() {
	delete m_pimpl;
}

PositionReporter & Strategy::GetPositionReporter() {
	if (!m_pimpl->m_positionReporter) {
		m_pimpl->m_positionReporter = CreatePositionReporter().release();
	}
	return *m_pimpl->m_positionReporter;
}

const std::string & Strategy::GetTypeName() const {
	static const std::string typeName = "Strategy";
	return typeName;
}

bool Strategy::IsBlocked() const {
	return m_pimpl->m_isBlocked || !GetSettings().IsValidPrice(GetSecurity());
}

void Strategy::CheckPositions(Notifier &notifier) {
	const Lock lock(GetMutex());
	if (m_pimpl->m_isBlocked) {
		return;
	}
	while (m_pimpl->CheckPositionsUnsafe(notifier));
}

//////////////////////////////////////////////////////////////////////////
