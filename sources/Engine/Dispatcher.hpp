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

		class AlgoState;
		class ObserverState;
		class Notifier;
		class Slots;

	public:

		Dispatcher(boost::shared_ptr<const Trader::Settings>);
		~Dispatcher();

	public:

		void Register(boost::shared_ptr<Algo>);
		void Register(boost::shared_ptr<Observer>);
		void CloseAll();

	public:

		void Start();
		void Stop();

	private:

	
		boost::shared_ptr<Notifier> m_notifier;
		std::unique_ptr<Slots> m_slots;
	
	};

} }
