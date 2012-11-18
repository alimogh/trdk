/**************************************************************************
 *   Created: 2012/07/09 16:09:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace Engine {

	class Dispatcher : private boost::noncopyable {

	private:

		class StrategyState;
		class TradeObserverState;
		class Notifier;
		class UpdateSlots;
		class TradesObservationSlots;

	public:

		Dispatcher(boost::shared_ptr<const Trader::Settings>);
		~Dispatcher();

	public:

		void SubscribeToLevel1(Trader::Strategy &);
		void SubscribeToLevel1(Trader::Service &);
		void SubscribeToLevel1(Trader::Observer &);

		void SubscribeToNewTrades(Trader::Strategy &);
		void SubscribeToNewTrades(Trader::Service &);
		void SubscribeToNewTrades(Trader::Observer &);

	public:

		void Start();
		void Stop();

	private:

		void SubscribeToNewTrades(
				boost::shared_ptr<TradeObserverState> &,
				const Trader::Security &);

	private:

		boost::shared_ptr<Notifier> m_notifier;
		std::unique_ptr<UpdateSlots> m_updateSlots;
		std::unique_ptr<TradesObservationSlots> m_tradesObservationSlots;
	
	};

} }
