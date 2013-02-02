/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SubscriptionsManager.hpp"
#include "Util.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"
#include "Core/Observer.hpp"

namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

	void Report(
				const Module &module,
				const Security &security,
				const char *type)
			throw () {
		module.GetLog().Info(
			"Subscribed to %1% from \"%2%\".",
			boost::make_tuple(
				type,
				boost::cref(security)));
	}

}

////////////////////////////////////////////////////////////////////////////////

SubscriptionsManager::SubscriptionsManager(Engine::Context &context)
		: m_dispatcher(context) {
	//...//
}

SubscriptionsManager::~SubscriptionsManager() {
	try {
		foreach (const auto &connection, m_slotConnections) {
			connection.disconnect();
		}
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

bool SubscriptionsManager::IsActive() const {
	return m_dispatcher.IsActive();
}

void SubscriptionsManager::Activate() {
	decltype(m_subscribedStrategies) emptyStrategies;
	m_dispatcher.Activate();
	emptyStrategies.swap(m_subscribedStrategies);
}

void SubscriptionsManager::Suspend() {
	m_dispatcher.Suspend();
}

void SubscriptionsManager::SubscribeToLevel1(
			const SubscriberPtrWrapper &subscriber,
			const Security &security,
			std::list<boost::signals2::connection> &slotConnections) {
	const auto slot = Security::Level1UpdateSlot(
		boost::bind(
			&Dispatcher::SignalLevel1Update,
			&m_dispatcher,
			subscriber,
			boost::cref(security)));
	const auto connection = security.SubcribeToLevel1(slot);
	try {
		slotConnections.push_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "Level 1");
}

void SubscriptionsManager::SubscribeToTrades(
			const SubscriberPtrWrapper &subscriber,
			const Security &security,
			std::list<boost::signals2::connection> &slotConnections) {
	const auto slot = Security::NewTradeSlot(
		boost::bind(
			&Dispatcher::SignalNewTrade,
			&m_dispatcher,
			subscriber,
			boost::cref(security),
			_1,
			_2,
			_3,
			_4));
	const auto connection = security.SubcribeToTrades(slot);
	try {
		slotConnections.push_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "new trades");
}

void SubscriptionsManager::SubscribeToLevel1(Strategy &strategy) {
	Assert(!IsActive());
	Subscribe(
		strategy,
		[this](
					const SubscriberPtrWrapper &subscriber,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1(Service &service) {
	Assert(!IsActive());
	Subscribe(
		service,
		[this](
					const SubscriberPtrWrapper &subscriber,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1(Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		observer,
		[this](
				const SubscriberPtrWrapper &subscriber,
				const Security &security,
				std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		subscriber,
		[this](
					const SubscriberPtrWrapper &subscriber,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		subscriber,
		[this](
					const SubscriberPtrWrapper &subscriber,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		observer,
		[this](
					const SubscriberPtrWrapper &subscriber,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(subscriber, security, slotConnections);
		});
}

void SubscriptionsManager::Subscribe(
			Strategy &observer,
			const SubscribeImpl &subscribeImpl) {
	if (m_subscribedStrategies.find(&observer) != m_subscribedStrategies.end()) {
		subscribeImpl(
			SubscriberPtrWrapper(observer),
			observer.GetSecurity(),
			m_slotConnections);
	} else {
		auto subscribedStrategies = m_subscribedStrategies;
		subscribedStrategies.insert(&observer);
		auto slotConnections = m_slotConnections;
		const auto positionUpdateConnection
			= observer.SubscribeToPositionsUpdates(
				boost::bind(
					&Dispatcher::SignalPositionUpdate,
					&m_dispatcher,
					SubscriberPtrWrapper(observer),
					_1));
		try {
			slotConnections.push_back(positionUpdateConnection);
			subscribeImpl(
				SubscriberPtrWrapper(observer),
				observer.GetSecurity(),
				slotConnections);
		} catch (...) {
			try {
				positionUpdateConnection.disconnect();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
			throw;
		}
		slotConnections.swap(m_slotConnections);
		subscribedStrategies.swap(m_subscribedStrategies);
	}
}

void SubscriptionsManager::Subscribe(
			Service &service,
			const SubscribeImpl &subscribeImpl) {

	struct ServiceSubscriberVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {

	public:

		explicit ServiceSubscriberVisitor(
					Dispatcher &dispatcher,
					std::list<boost::signals2::connection> &connections,
					std::set<const Strategy *> &subscribedStrategies)
				: m_dispatcher(dispatcher),
				m_connections(connections),
				m_subscribed(subscribedStrategies) {
			//...//
		}

	public:

		void operator ()(Strategy &strategy) const {
			if (m_subscribed.find(&strategy) != m_subscribed.end()) {
				return;
			}
			const auto positionUpdateConnection
				= strategy.SubscribeToPositionsUpdates(
					boost::bind(
						&Dispatcher::SignalPositionUpdate,
						&m_dispatcher,
						SubscriberPtrWrapper(strategy),
						_1));
			try {
				m_connections.push_back(positionUpdateConnection);
			} catch (...) {
				try {
					positionUpdateConnection.disconnect();
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				throw;
			}
			m_subscribed.insert(&strategy);
		}
		void operator ()(Service &service) const {
			foreach (const auto &serviceSubscriber, service.GetSubscribers()) {
				boost::apply_visitor(*this, serviceSubscriber);
			}
		}
		void operator ()(const Observer &) const {
			//...//
		}

	private:

		Dispatcher &m_dispatcher;
		std::list<boost::signals2::connection> &m_connections;
		std::set<const Strategy *> &m_subscribed;

	};

	decltype(m_slotConnections) newSlotConnections;
	auto slotConnections = m_slotConnections;
	auto subscribedStrategies = m_subscribedStrategies;
	
	ServiceSubscriberVisitor serviceSubscriberVisitor(
		m_dispatcher,
		newSlotConnections,
		m_subscribedStrategies);
	try {
		foreach (const auto &serviceSubscriber, service.GetSubscribers()) {
			boost::apply_visitor(serviceSubscriberVisitor, serviceSubscriber);
		}
		subscribeImpl(
			SubscriberPtrWrapper(service),
			service.GetSecurity(),
			newSlotConnections);
		std::copy(
			newSlotConnections.begin(),
			newSlotConnections.end(),
			std::back_inserter(slotConnections));
	} catch (...) {
		try {
			foreach (const auto &connection, newSlotConnections) {
				connection.disconnect();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}

	subscribedStrategies.swap(m_subscribedStrategies);
	slotConnections.swap(m_slotConnections);

}

void SubscriptionsManager::Subscribe(
			Observer &observer,
			const SubscribeImpl &subscribeImpl) {
	auto globalSlotConnections = m_slotConnections;
	std::list<boost::signals2::connection> slotConnections;
	try {
		foreach (auto &security, observer.GetNotifyList()) {
			subscribeImpl(
				SubscriberPtrWrapper(observer),
				*security,
				slotConnections);
		}
		std::copy(
			slotConnections.begin(),
			slotConnections.end(),
			std::back_inserter(globalSlotConnections));
	} catch (...) {
		try {
			foreach (const auto &connection, slotConnections) {
				connection.disconnect();
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	globalSlotConnections.swap(m_slotConnections);
}

////////////////////////////////////////////////////////////////////////////////
