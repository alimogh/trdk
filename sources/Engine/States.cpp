/**************************************************************************
 *   Created: 2012/11/22 11:46:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "States.hpp"
#include "Notifier.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Settings.hpp"

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////

	struct GetModuleVisitor : public boost::static_visitor<Module &> {
		template<typename ModulePtr>
		Module & operator ()(ModulePtr &module) const {
			return *module;
		}
		template<>
		Module & operator ()(boost::shared_ptr<StrategyState> &module) const {
			return module->GetStrategy();
		}
	};

	//////////////////////////////////////////////////////////////////////////

	class NotifyServiceDataUpdateVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:
		explicit NotifyServiceDataUpdateVisitor(const Service &service)
				: m_service(service) {
			//...//
		}
	public:		
		template<typename Module>
		void operator ()(const boost::shared_ptr<Module> &module) const {
			NotifyObserver(*module);
		}
		template<>
		void operator ()(const boost::shared_ptr<Service> &module) const {
			NotifyObserver(*module);
			Engine::Detail::NotifyServiceDataUpdateSubscribers(*module);
		}
	public:
		template<typename Module>
		void NotifyObserver(Module &module) const {
			const Module::Lock lock(module.GetMutex());
			module.OnServiceDataUpdate(m_service);
		}
	private:
		const Service &m_service;
	};

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

namespace Trader { namespace Engine { namespace Detail {

	void NotifyServiceDataUpdateSubscribers(const Service &service) {
		const NotifyServiceDataUpdateVisitor visitor(service);
		foreach (auto subscriber, service.GetSubscribers()) {
			try {
				boost::apply_visitor(visitor, subscriber);
			} catch (const MethodDoesNotImplementedError &ex) {
				Log::Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(GetModuleVisitor(), subscriber));
				throw;
			} catch (const Exception &ex) {
				Log::Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(GetModuleVisitor(), subscriber));
			}
		}
	}

} } }

//////////////////////////////////////////////////////////////////////////

StrategyState::StrategyState(
			Strategy &startegy,
			boost::shared_ptr<Notifier> notifier,
			boost::shared_ptr<const Settings> settings)
		: m_strategy(startegy.shared_from_this()),
		m_notifier(notifier),
		m_isBlocked(false),
		m_lastUpdate(0),
		m_settings(settings) {
	//...//
}

void StrategyState::CheckPositions(bool byTimeout) {
	Assert(!m_settings->IsReplayMode() || !byTimeout);
	if (byTimeout && !IsTimeToUpdate()) {
		return;
	}
	const Strategy::Lock lock(m_strategy->GetMutex());
	if (m_isBlocked || (byTimeout && !IsTimeToUpdate())) {
		return;
	}
	while (CheckPositionsUnsafe());
}

bool StrategyState::IsTimeToUpdate() const {
	if (	(IsBlocked() || !m_lastUpdate)
			&& m_settings->ShouldWaitForMarketData()) {
		return false;
	}
	const auto now
		= (boost::get_system_time() - m_settings->GetStartTime())
			.total_milliseconds();
	AssertGe(now, m_lastUpdate);
	const auto diff = boost::uint64_t(now - m_lastUpdate);
	return diff >= m_settings->GetUpdatePeriodMilliseconds();
}

bool StrategyState::IsBlocked() const {
	return m_isBlocked || !m_strategy->IsValidPrice(*m_settings);
}

bool StrategyState::CheckPositionsUnsafe() {

	const auto now = boost::get_system_time();
	Interlocking::Exchange(
		m_lastUpdate,
		(now - m_settings->GetStartTime()).total_milliseconds());

	Assert(!m_isBlocked);
		
	const Security &security = *m_strategy->GetSecurity();
	Assert(security || !m_settings->ShouldWaitForMarketData()); // must be checked it security object

	if (m_positions) {
		Assert(!m_positions->Get().empty());
		Assert(m_stateUpdateConnections.IsConnected());
		ReportClosedPositon(*m_positions);
		if (!m_positions->IsCompleted()) {
			if (m_positions->IsOk()) {
				if (	!m_settings->IsReplayMode()
						&& m_settings->GetCurrentTradeSessionEndime() <= now) {
					foreach (auto &p, m_positions->Get()) {
						if (p->IsCanceled()) {
							continue;
						}
						p->CancelAtMarketPrice(
							Position::CLOSE_TYPE_SCHEDULE);
					}
				} else {
					m_strategy->TryToClosePositions(*m_positions);
				}
			} else {
				Log::Warn(
					"Strategy \"%1%\" BLOCKED by dispatcher.",
					m_strategy->GetName());
				Interlocking::Exchange(m_isBlocked, true);
			}
			return false;
		}
		m_stateUpdateConnections.Diconnect();
		m_positions.reset();
	}

	Assert(!m_stateUpdateConnections.IsConnected());

	boost::shared_ptr<PositionBandle> positions
		= m_strategy->TryToOpenPositions();
	if (!positions || positions->Get().empty()) {
		return false;
	}

	StateUpdateConnections stateUpdateConnections;
	foreach (const auto &p, positions->Get()) {
		Assert(&p->GetSecurity() == &security);
		m_strategy->ReportDecision(*p);
		stateUpdateConnections.InsertSafe(
			p->Subscribe(
				boost::bind(
					&Notifier::Signal,
					m_notifier.get(),
					shared_from_this())));
	}
	Assert(stateUpdateConnections.IsConnected());

	stateUpdateConnections.Swap(m_stateUpdateConnections);
	positions.swap(m_positions);
	return true;

}

void StrategyState::ReportClosedPositon(PositionBandle &positions) {
	Assert(!positions.Get().empty());
	foreach (auto &p, positions.Get()) {
		if (	p->IsOpened()
				&& !p->IsReported()
				&& (p->IsClosed() || p->IsError())) {
			m_strategy->GetPositionReporter().ReportClosedPositon(*p);
			p->MarkAsReported();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void TradeObserverState::NotifyVisitor::NotifyObserver(SecurityAlgo &observer) const {
	AssertEq(
		m_trade.security->GetFullSymbol(),
		observer.GetSecurity()->GetFullSymbol());
	const Module::Lock lock(observer.GetMutex());
	observer.OnNewTrade(
		m_trade.time,
		m_trade.price,
		m_trade.qty,
		m_trade.side);
}

void TradeObserverState::NotifyVisitor::NotifyObserver(Observer &observer) const {
	const Module::Lock lock(observer.GetMutex());
	observer.OnNewTrade(
		*m_trade.security,
		m_trade.time,
		m_trade.price,
		m_trade.qty,
		m_trade.side);
}

//////////////////////////////////////////////////////////////////////////

Module & TradeObserverState::GetObserver() {
	return boost::apply_visitor(GetModuleVisitor(), m_observer);
}

//////////////////////////////////////////////////////////////////////////
