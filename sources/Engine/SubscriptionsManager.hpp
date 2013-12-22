/**************************************************************************
 *   Created: 2012/07/09 16:09:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Dispatcher.hpp"
#include "Fwd.hpp"

namespace trdk { namespace Engine {

	class SubscriptionsManager : private boost::noncopyable {

	private:

		typedef std::function<
				void (
					Security &,
					const SubscriberPtrWrapper &,
					std::list<boost::signals2::connection> &)>
			SubscribeImpl;

	public:

		explicit SubscriptionsManager(Context &);
		~SubscriptionsManager();

	public:

		void SubscribeToBrokerPositionUpdates(
					trdk::Security &,
					trdk::Strategy &);
		void SubscribeToBrokerPositionUpdates(
					trdk::Security &,
					trdk::Service &);
		void SubscribeToBrokerPositionUpdates(
					trdk::Security &,
					trdk::Observer &);

		void SubscribeToBars(trdk::Security &, trdk::Strategy &);
		void SubscribeToBars(trdk::Security &, trdk::Service &);
		void SubscribeToBars(trdk::Security &, trdk::Observer &);

	public:

		bool IsActive() const;
		void Activate();
		void Suspend();

	private:

		void SubscribeToBrokerPositionUpdates(
					Security &,
					const SubscriberPtrWrapper &,
					std::list<boost::signals2::connection> &);

		void Subscribe(Security &, Strategy &, const SubscribeImpl &);
		void Subscribe(Security &, Service &, const SubscribeImpl &);
		void Subscribe(Security &, Observer &, const SubscribeImpl &);

	private:

		Dispatcher m_dispatcher;
		std::list<boost::signals2::connection> m_slotConnections;

		std::set<const Strategy *> m_subscribedStrategies;
	
	};

} }
