/**************************************************************************
 *   Created: 2012/07/09 16:09:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Dispatcher.hpp"
#include "Fwd.hpp"

namespace Trader { namespace Engine {

	class SubscriptionsManager : private boost::noncopyable {

	private:

		typedef std::function<
				void (
					const SubscriberPtrWrapper &,
					const Security &,
					std::list<boost::signals2::connection> &)>
			SubscribeImpl;

	public:

		explicit SubscriptionsManager(boost::shared_ptr<const Trader::Settings>);
		~SubscriptionsManager();

	public:

		void SubscribeToLevel1(Trader::Strategy &);
		void SubscribeToLevel1(Trader::Service &);
		void SubscribeToLevel1(Trader::Observer &);

		void SubscribeToTrades(Trader::Strategy &);
		void SubscribeToTrades(Trader::Service &);
		void SubscribeToTrades(Trader::Observer &);

	public:

		bool IsActive() const;
		void Activate();
		void Suspend();

	private:

		void SubscribeToLevel1(
					const SubscriberPtrWrapper &,
					const Security &,
					std::list<boost::signals2::connection> &);
		void SubscribeToTrades(
					const SubscriberPtrWrapper &,
					const Security &,
					std::list<boost::signals2::connection> &);

		void Subscribe(Strategy &, const SubscribeImpl &);
		void Subscribe(Service &, const SubscribeImpl &);
		void Subscribe(Observer &, const SubscribeImpl &);

	private:

		Dispatcher m_dispatcher;
		std::list<boost::signals2::connection> m_slotConnections;

		std::set<const Strategy *> m_subscribedStrategies;
	
	};

} }
