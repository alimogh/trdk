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
		class ObserverState;
		class Notifier;
		class UpdateSlots;
		class ObservationSlots;

	public:

		Dispatcher(boost::shared_ptr<const Trader::Settings>);
		~Dispatcher();

	public:

		void SubscribeToLevel1(Strategy &);
		void SubscribeToLevel1(Service &);
		void SubscribeToLevel1(Observer &);

		void SubscribeToNewTrades(Strategy &);
		void SubscribeToNewTrades(Service &);
		void SubscribeToNewTrades(Observer &);

	public:

		void Start();
		void Stop();

	private:

		boost::shared_ptr<Notifier> m_notifier;
		std::unique_ptr<UpdateSlots> m_updateSlots;
		std::unique_ptr<ObservationSlots> m_observationSlots;
	
	};

} }
