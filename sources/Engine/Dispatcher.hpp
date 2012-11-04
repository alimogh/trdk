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
		class Slots;
		class ObservationSlots;

	public:

		Dispatcher(boost::shared_ptr<const Trader::Settings>);
		~Dispatcher();

	public:

		void Register(boost::shared_ptr<Strategy>);
		void Register(boost::shared_ptr<Observer>);
		void Register(boost::shared_ptr<Service>);

	public:

		void Start();
		void Stop();

	private:

	
		boost::shared_ptr<Notifier> m_notifier;
		std::unique_ptr<Slots> m_slots;
		std::unique_ptr<ObservationSlots> m_observationSlots;
	
	};

} }
