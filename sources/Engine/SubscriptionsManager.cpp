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
		Log::Info(
			"\"%1%\" subscribed to %2% from \"%3%\".",
			module,
			type,
			security);
	}

}

////////////////////////////////////////////////////////////////////////////////

SubscriptionsManager::SubscriptionsManager(
			boost::shared_ptr<const Settings> options)
		: m_dispatcher(options) {
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

void SubscriptionsManager::Start() {
	m_dispatcher.Start();
}

void SubscriptionsManager::Stop() {
	m_dispatcher.Stop();
}

void SubscriptionsManager::SubscribeToLevel1(
			boost::shared_ptr<Notifier> notifier,
			const Security &security,
			std::list<boost::signals2::connection> &slotConnections) {
	const auto slot = Security::Level1UpdateSlot(
		boost::bind(
			&Dispatcher::SignalLevel1Update,
			&m_dispatcher,
			notifier,
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
	Report(notifier->GetObserver(), security, "Level 1");
}

void SubscriptionsManager::SubscribeToTrades(
			boost::shared_ptr<Notifier> notifier,
			const Security &security,
			std::list<boost::signals2::connection> &slotConnections) {
	const auto slot = Security::NewTradeSlot(
		boost::bind(
			&Dispatcher::SignalNewTrade,
			&m_dispatcher,
			notifier,
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
	Report(notifier->GetObserver(), security, "new trades");
}

void SubscriptionsManager::SubscribeToLevel1(Strategy &strategy) {
	Subscribe(
		strategy,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1(Service &service) {
	Subscribe(
		service,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1(Observer &observer) {
	Subscribe(
		observer,
		[this](
				boost::shared_ptr<Notifier> notifier,
				const Security &security,
				std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Strategy &observer) {
	Subscribe(
		observer,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Service &observer) {
	Subscribe(
		observer,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Observer &observer) {
	Subscribe(
		observer,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::Subscribe(
			Strategy &observer,
			const SubscribeImpl &subscribeImpl) {
	boost::shared_ptr<Notifier> notifier(
		new Notifier(observer.shared_from_this()));
	if (m_subscribedStrategies.find(&observer) != m_subscribedStrategies.end()) {
		subscribeImpl(notifier, observer.GetSecurity(), m_slotConnections);
	} else {
		auto subscribedStrategies = m_subscribedStrategies;
		subscribedStrategies.insert(&observer);
		auto slotConnections = m_slotConnections;
		const auto positionUpdateConnection
			= observer.SubscribeToPositionsUpdates(
				boost::bind(
					&Dispatcher::SignalPositionUpdate,
					&m_dispatcher,
					notifier,
					_1));
		try {
			slotConnections.push_back(positionUpdateConnection);
			subscribeImpl(notifier, observer.GetSecurity(), slotConnections);
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

		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			if (m_subscribed.find(&*strategy) != m_subscribed.end()) {
				return;
			}
			boost::shared_ptr<Notifier> notifier(new Notifier(strategy));
			const auto positionUpdateConnection
				= strategy->SubscribeToPositionsUpdates(
					boost::bind(
						&Dispatcher::SignalPositionUpdate,
						&m_dispatcher,
						notifier,
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
			m_subscribed.insert(&*strategy);
		}
		void operator ()(const boost::shared_ptr<Service> &service) const {
			foreach (const auto &serviceSubscriber, service->GetSubscribers()) {
				boost::apply_visitor(*this, serviceSubscriber);
			}
		}
		void operator ()(const boost::shared_ptr<Observer> &) const {
			//...//
		}

	private:

		Dispatcher &m_dispatcher;
		std::list<boost::signals2::connection> &m_connections;
		std::set<const Strategy *> &m_subscribed;

	};

	boost::shared_ptr<Notifier> notifier(
		new Notifier(service.shared_from_this()));

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
		subscribeImpl(notifier, service.GetSecurity(), newSlotConnections);
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
	boost::shared_ptr<Notifier> notifier(
		new Notifier(observer.shared_from_this()));
	std::list<boost::signals2::connection> slotConnections;
	try {
		foreach (auto &security, observer.GetNotifyList()) {
			subscribeImpl(notifier, *security, slotConnections);
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
