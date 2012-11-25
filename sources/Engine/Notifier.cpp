/**************************************************************************
 *   Created: 2012/11/22 11:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Notifier.hpp"
#include "Core/Strategy.hpp"
#include "Core/Settings.hpp"

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;


Notifier::Notifier(boost::shared_ptr<const Settings> settings)
			: m_isActive(false),
			m_isExit(false),
			m_currentLevel1Updates(&m_level1UpdatesQueue.first),
			m_heavyLoadedUpdatesCount(0),
			m_currentTradeObservervation(&m_tradeObservervationQueue.first),
			m_heavyLoadedObservationsCount(0),
			m_settings(settings) {
		
	{
		const char *const threadName = "Strategy";
		Log::Info(
			"Starting %1% thread(s) \"%2%\"...",
			m_settings->GetThreadsCount(),
			threadName);
		boost::barrier startBarrier(m_settings->GetThreadsCount() + 1);
		for (size_t i = 0; i < m_settings->GetThreadsCount(); ++i) {
			m_threads.create_thread(
				[&]() {
					Task(
						startBarrier,
						threadName,
						&Notifier::NotifyLevel1Update);
				});
		}
		startBarrier.wait();
		Log::Info("All \"%1%\" threads started.", threadName);
	}
	{
		boost::barrier startBarrier(1 + 1);
		m_threads.create_thread(
			[&]() {
				Task(startBarrier, "Observer", &Notifier::NotifyNewTrades);
			});
		startBarrier.wait();
	}
}

Notifier::~Notifier() {
	try {
		{
			const Lock lock(m_mutex);
			Assert(!m_isExit);
			m_isExit = true;
			m_isActive = false;
			m_positionsCheckCompletedCondition.notify_all();
			m_tradeReadCompletedCondition.notify_all();
			m_positionsCheckCondition.notify_all();
			m_newTradesCondition.notify_all();
		}
		m_threads.join_all();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

bool Notifier::NotifyLevel1Update() {

	Level1UpdateNotifyList *notifyList = nullptr;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentLevel1Updates == &m_level1UpdatesQueue.first
			|| m_currentLevel1Updates == &m_level1UpdatesQueue.second);
		if (m_currentLevel1Updates->empty()) {
			m_heavyLoadedUpdatesCount = 0;
			m_positionsCheckCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentLevel1Updates->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher updates thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentLevel1Updates;
		m_currentLevel1Updates = m_currentLevel1Updates == &m_level1UpdatesQueue.first
			?	&m_level1UpdatesQueue.second
			:	&m_level1UpdatesQueue.first;
		if (m_settings->IsReplayMode()) {
			m_positionsCheckCompletedCondition.notify_all();
		}
	}

	Assert(!notifyList->empty());
	foreach (auto &notification, *notifyList) {
		notification->CheckPositions();
	}
	notifyList->clear();

	return true;

}

bool Notifier::NotifyNewTrades() {

	TradeNotifyList *notifyList = nullptr;
	{

		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentTradeObservervation == &m_tradeObservervationQueue.first
			|| m_currentTradeObservervation == &m_tradeObservervationQueue.second);
		if (m_currentTradeObservervation->empty()) {
			m_heavyLoadedObservationsCount = 0;
			m_newTradesCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentTradeObservervation->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher observers thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentTradeObservervation;
		m_currentTradeObservervation = m_currentTradeObservervation == &m_tradeObservervationQueue.first
			?	&m_tradeObservervationQueue.second
			:	&m_tradeObservervationQueue.first;
		if (m_settings->IsReplayMode()) {
			m_tradeReadCompletedCondition.notify_all();
		}
	}

	Assert(!notifyList->empty());
	foreach (auto &observationEvent, *notifyList) {
		observationEvent.state->NotifyNewTrades(observationEvent);
	}
	notifyList->clear();

	return true;

}

void Notifier::Signal(boost::shared_ptr<Strategy> strategy) {
		
	if (strategy->IsBlocked()) {
		return;
	}
		
	Lock lock(m_mutex);
	if (m_isExit) {
		return;
	}
	//! @todo place for optimization
	if (	m_currentLevel1Updates->find(strategy)
			!= m_currentLevel1Updates->end()) {
		return;
	}
	m_currentLevel1Updates->insert(strategy);
	m_positionsCheckCondition.notify_one();
	if (m_settings->IsReplayMode()) {
		m_positionsCheckCompletedCondition.wait(lock);
	}

}

void Notifier::Signal(
			boost::shared_ptr<TradeObserverState> state,
			const boost::shared_ptr<const Security> &security,
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const Trade trade = {
		state,
		security,
		time,
		price,
		qty,
		side};
	Lock lock(m_mutex);
	if (m_isExit) {
		return;
	}
	m_currentTradeObservervation->push_back(trade);
	m_newTradesCondition.notify_one();
	if (m_settings->IsReplayMode()) {
		m_tradeReadCompletedCondition.wait(lock);
	}
}
