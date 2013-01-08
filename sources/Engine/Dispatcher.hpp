/**************************************************************************
 *   Created: 2012/11/22 11:45:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Notifier.hpp"

namespace Trader { namespace Engine {

	class Dispatcher : private boost::noncopyable {

	private:

		template<typename ListT>
		class EventQueue : private boost::noncopyable {

		public:

			typedef ListT List;

		private:

			typedef boost::mutex Mutex;
			typedef Mutex::scoped_lock Lock;
			typedef boost::condition_variable Condition;

		public:
			
			EventQueue(
						const char *name,
						boost::shared_ptr<const Settings> &settings)
					: m_name(name),
					m_current(&m_lists.first),
					m_heavyLoadsCount(0),
					m_isExit(false),
					m_isActive(false) {
				if (settings->IsReplayMode()) {
					m_readyToReadCondition.reset(new Condition);
				}
			}

		public:

			const char * GetName() const {
				return m_name;
			}

			void Start() {
				const Lock lock(m_mutex);
				m_isActive = true;
			}

			void Stop() {
				const Lock lock(m_mutex);
				m_isActive = false;
			}

			template<typename Event>
			void Queue(const Event &event) {
				Lock lock(m_mutex);
				if (m_isExit) {
					return;
				}
				Assert(
					m_current == &m_lists.first
					|| m_current == &m_lists.second);
				if (!Dispatcher::QueueEvent(event, *m_current)) {
					return;
				}
				m_newDataCondition.notify_one();
				if (m_readyToReadCondition) {
					m_readyToReadCondition->wait(lock);
				}
			}

			bool Enqueue() {

				List *listToRead;
				
				{

					Lock lock(m_mutex);
				
					if (m_isExit) {
						return false;
					}
				
					Assert(m_current == &m_lists.first || m_current == &m_lists.second);
					if (m_current->empty()) {
						m_heavyLoadsCount = 0;
						m_newDataCondition.wait(lock);
						if (m_isExit) {
							return false;
						} else if (m_current->empty() || !m_isActive) {
							return true;
						}
					} else if (!(++m_heavyLoadsCount % 100)) {
						Log::Warn(
							"Dispatcher task \"%1%\" is heavy loaded (%1% iterations)!",
							m_name,
							m_heavyLoadsCount);
					}

					listToRead = m_current;
					m_current = m_current == &m_lists.first
						?	&m_lists.second
						:	&m_lists.first;
					if (m_readyToReadCondition) {
						m_readyToReadCondition->notify_all();
					}

				}

				Assert(!listToRead->empty());
				foreach(const auto &event, *listToRead) {
					Dispatcher::RaiseEvent(event);
				}
				listToRead->clear();
			
				return true;

			}

		private:

			const char *const m_name;

			std::pair<List, List> m_lists;
			List *m_current;
			size_t m_heavyLoadsCount;
			
			Mutex m_mutex;
			Condition m_newDataCondition;
			std::unique_ptr<Condition> m_readyToReadCondition;

			bool m_isExit;
			bool m_isActive;

		};

		typedef boost::tuple<const Security *, boost::shared_ptr<Notifier>>
			Level1UpdateEvent;
		typedef EventQueue<std::set<Level1UpdateEvent>> Level1UpdateEventQueue;

		typedef boost::tuple<
				boost::shared_ptr<Notifier::Trade>,
				boost::shared_ptr<Notifier>>
			NewTradeEvent;
		//! @todo	HAVY OPTIMIZATION!!! Use preallocated buffer here instead
		//!			std::list.
		typedef EventQueue<std::list<NewTradeEvent>> NewTradeEventQueue;

		typedef boost::tuple<Position *, boost::shared_ptr<Notifier>>
			PositionUpdateEvent;
		typedef EventQueue<std::set<PositionUpdateEvent>>
			PositionUpdateEventQueue;

	public:

		explicit Dispatcher(boost::shared_ptr<const Settings> &settings);
		~Dispatcher();

	public:

		void Start() {
			Log::Info("Starting events dispatching...");
			m_positionUpdates.Start();
			m_level1Updates.Start();
			m_newTrades.Start();
			Log::Info("Events dispatching started.");
		}

		void Stop() {
			Log::Info("Stopping events dispatching...");
			m_newTrades.Stop();
			m_level1Updates.Stop();
			m_positionUpdates.Stop();
			Log::Info("Events dispatching stopped.");
		}

	public:

		void SignalLevel1Update(boost::shared_ptr<Notifier> &, const Security &);
		void SignalNewTrade(
					boost::shared_ptr<Notifier> &,
					const Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide);
		virtual void SignalPositionUpdate(
					boost::shared_ptr<Notifier> &,
					Position &);

	private:

		template<typename Event>
		static void RaiseEvent(const Event &) {
			static_assert(false, "Failed to find event raise specialization.");
		}
		template<>
		static void RaiseEvent(const Level1UpdateEvent &level1Update) {
			boost::get<1>(level1Update)->RaiseLevel1UpdateEvent(
				*boost::get<0>(level1Update));
		}
		template<>
		static void RaiseEvent(const NewTradeEvent &newTradeEvent) {
			boost::get<1>(newTradeEvent)->RaiseNewTradeEvent(
				*boost::get<0>(newTradeEvent));
		}
		template<>
		static void RaiseEvent(const PositionUpdateEvent &positionUpdateEvent) {
			boost::get<1>(positionUpdateEvent)->RaisePositionUpdateEvent(
				*boost::get<0>(positionUpdateEvent));
		}

		template<typename Event, typename EventList>
		static bool QueueEvent(const Event &, EventList &) {
			static_assert(false, "Failed to find event queue specialization.");
		}
		template<typename EventList>
		static bool QueueEvent(
					const Level1UpdateEvent &level1UpdateEvent,
					EventList &eventList) {
			//! @todo place for optimization
			if (eventList.find(level1UpdateEvent) != eventList.end()) {
				return false;
			}
			eventList.insert(level1UpdateEvent);
			return true;
		}
		template<typename EventList>
		static bool QueueEvent(
					const NewTradeEvent &newTradeEvent,
					EventList &eventList) {
			eventList.push_back(newTradeEvent);
			return true;
		}
		template<typename EventList>
		static bool QueueEvent(
					const PositionUpdateEvent &positionUpdateEvent,
					EventList &eventList) {
			//! @todo place for optimization
			if (eventList.find(positionUpdateEvent) != eventList.end()) {
				return false;
			}
			eventList.insert(positionUpdateEvent);
			return true;
		}

	private:

		template<typename EventList>
		void StartNotificationTask(EventList &list) {
			m_threads.create_thread(
				boost::bind(
					&Dispatcher::NotificationTask<EventList>,
					this,
					boost::ref(list)));
		}

		template<typename EventList>
		void NotificationTask(EventList &eventList) {
			Log::Info(
				"Dispatcher notification task \"%1%\" started...",
				eventList.GetName());
			bool isError = false;
			for ( ; ; ) {
				try {
					if (!eventList.Enqueue()) {
						break;
					}
				} catch (const Trader::Lib::ModuleError &ex) {
					Log::Error(
						"Module error in dispatcher notification task"
							" \"%1%\": \"%2%\".",
						eventList.GetName(),
						ex);
					isError = true;
					break;
				} catch (...) {
					Log::Error(
						"Unhandled exception caught in dispatcher"
							" notification task \"%1%\".",
						eventList.GetName());
					AssertFailNoException();
					isError = true;
					break;
				}
			}
			Log::Info(
				"Dispatcher notification task \"%1%\" stopped.",
				eventList.GetName());
			if (isError) {
				//! @todo: Call engine instance stop instead.
				exit(1);
			}
		}

	private:

		boost::thread_group m_threads;

		Level1UpdateEventQueue m_level1Updates;
		NewTradeEventQueue m_newTrades;
		PositionUpdateEventQueue m_positionUpdates;

	};

} }
