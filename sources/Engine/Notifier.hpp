/**************************************************************************
 *   Created: 2012/11/22 11:45:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "States.hpp"

namespace Trader { namespace Engine {

	class Notifier : private boost::noncopyable {

	public:

		typedef std::list<boost::shared_ptr<Strategy>> StrategyList;
		typedef std::list<
				boost::shared_ptr<TradeObserverState>>
			TradeObserverStateList;
	
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	private:

		typedef boost::condition_variable Condition;

		typedef std::set<boost::shared_ptr<Strategy>> Level1UpdateNotifyList;

		typedef std::list<Trade> TradeNotifyList;

	public:

		explicit Notifier(boost::shared_ptr<const Settings> settings);

		~Notifier();

		boost::shared_ptr<const Settings> GetSettings() const {
			return m_settings;
		}

	public:

		void Start() {
			const Lock lock(m_mutex);
			if (m_isActive) {
				return;
			}
			Log::Info("Starting events dispatching...");
			m_isActive = true;
		}

		void Stop() {
			const Lock lock(m_mutex);
			m_isActive = false;
		}

	public:

		Mutex & GetStrategyListMutex() {
			return m_strategyListMutex;
		}

		StrategyList & GetStrategyList() {
			return m_strategyList;
		}

		TradeObserverStateList & GetTradeObserverStateList() {
			return m_tradeObserverStateList;
		}

		void Signal(boost::shared_ptr<Strategy> strategyState);

		void Signal(
					boost::shared_ptr<TradeObserverState> state,
					const boost::shared_ptr<const Security> &security,
					const boost::posix_time::ptime &time,
					ScaledPrice price,
					Qty qty,
					OrderSide side);

	private:

		template<typename Iteration>
		void Task(
					boost::barrier &startBarrier,
					const char *const name,
					Iteration iteration) {
			startBarrier.wait();
			Log::Info("Dispatcher task \"%1%\" started...", name);
			for ( ; ; ) {
				try {
					if (!(this->*iteration)()) {
						break;
					}
				} catch (...) {
					Log::Error("Unhandled exception caught in dispatcher task \"%1%\".", name);
					AssertFailNoException();
					throw;
				}
			}
			Log::Info("Dispatcher task \"%1%\" stopped.", name);
		}

		bool NotifyLevel1Update();
		bool NotifyNewTrades();

	private:

		bool m_isActive;
		bool m_isExit;

		Mutex m_mutex;
		Condition m_positionsCheckCondition;
		Condition m_positionsCheckCompletedCondition;
		Condition m_tradeReadCompletedCondition;

		Condition m_newTradesCondition;

		std::pair<
				Level1UpdateNotifyList,
				Level1UpdateNotifyList>
			m_level1UpdatesQueue;
		Level1UpdateNotifyList *m_currentLevel1Updates;
		size_t m_heavyLoadedUpdatesCount;

		std::pair<TradeNotifyList, TradeNotifyList> m_tradeObservervationQueue;
		TradeNotifyList *m_currentTradeObservervation;
		size_t m_heavyLoadedObservationsCount;

		boost::shared_ptr<const Settings> m_settings;

		Mutex m_strategyListMutex;
		StrategyList m_strategyList;
	
		TradeObserverStateList m_tradeObserverStateList;

		boost::thread_group m_threads;

	};

} }
