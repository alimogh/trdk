/**************************************************************************
 *   Created: 2012/11/22 11:43:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Core/Service.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"

namespace Trader { namespace Engine {

	//////////////////////////////////////////////////////////////////////////

	namespace Detail {
		void NotifyServiceDataUpdateSubscribers(const Service &);
	}

	//////////////////////////////////////////////////////////////////////////

	class StrategyState
			: private boost::noncopyable,
			public boost::enable_shared_from_this<StrategyState> {

	private:

		typedef SignalConnectionList<Position::StateUpdateConnection>
			StateUpdateConnections;

	public:

		explicit StrategyState(
					Strategy &startegy,
					boost::shared_ptr<Notifier> notifier,
					boost::shared_ptr<const Settings> settings);

	public:

		Strategy & GetStrategy() {
			return *m_strategy;
		}

	public:

		void CheckPositions(bool byTimeout);

		bool IsTimeToUpdate() const;

		bool IsBlocked() const;

	private:

		bool CheckPositionsUnsafe();

	private:

		void ReportClosedPositon(PositionBandle &);

	private:

		boost::shared_ptr<Strategy> m_strategy;
		boost::shared_ptr<PositionBandle> m_positions;
		
		boost::shared_ptr<Notifier> m_notifier;
		StateUpdateConnections m_stateUpdateConnections;

		volatile int64_t m_isBlocked;

		volatile int64_t m_lastUpdate;

		boost::shared_ptr<const Settings> m_settings;

	};

	//////////////////////////////////////////////////////////////////////////

	class TradeObserverState
			: private boost::noncopyable,
			public boost::enable_shared_from_this<TradeObserverState> {

	public:

		struct Trade {
			boost::shared_ptr<TradeObserverState> state;
			boost::shared_ptr<const Security> security;
			boost::posix_time::ptime time;
			ScaledPrice price;
			Qty qty;
			OrderSide side;
		};

	private:

		class NotifyVisitor
				: public boost::static_visitor<void>,
				private boost::noncopyable {
		public:		
			explicit NotifyVisitor(const Trade &trade)
					: m_trade(trade) {
				//...//
			}
			template<typename Module>
			void operator ()(
						const boost::shared_ptr<Module> &observer)
					const {
				NotifyObserver(*observer);
			}
			template<>
			void operator ()(
						const boost::shared_ptr<StrategyState> &state)
					const {
				state->CheckPositions(false);
			}
			template<>
			void operator ()(const boost::shared_ptr<Service> &observer) const {
				NotifyObserver(*observer);
				Detail::NotifyServiceDataUpdateSubscribers(*observer);
			}
		private:
			void NotifyObserver(SecurityAlgo &) const;
			void NotifyObserver(Observer &) const;
		private:
			const Trade &m_trade;
		};

	public:

		template<typename Module>
		explicit TradeObserverState(boost::shared_ptr<Module> observer)
				: m_observer(observer) {
			//...//
		}

		void NotifyNewTrades(const Trade &trade) {
			boost::apply_visitor(NotifyVisitor(trade), m_observer);
		}

		Module & GetObserver();

	private:

		boost::variant<
				boost::shared_ptr<StrategyState>,
				boost::shared_ptr<Service>,
				boost::shared_ptr<Observer>>
			m_observer;

	};

	//////////////////////////////////////////////////////////////////////////

} }
