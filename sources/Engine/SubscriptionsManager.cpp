/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SubscriptionsManager.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"
#include "Core/Observer.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace sig = boost::signals2;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

	void Report(
				const Module &module,
				Security &security,
				const char *type)
			throw () {
		module.GetLog().Info(
			"Subscribed to %1% from \"%2%\" (source: \"%3%\").",
			type,
			security,
			security.GetSource().GetTag());
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
		terminate();
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

void SubscriptionsManager::SubscribeToLevel1Updates(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::Level1UpdateSlot(
		boost::bind(
			&Dispatcher::SignalLevel1Update,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1));
	const auto connection = security.SubscribeToLevel1Updates(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "level 1 updates");
}

void SubscriptionsManager::SubscribeToLevel1Ticks(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::Level1TickSlot(
		boost::bind(
			&Dispatcher::SignalLevel1Tick,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1,
			_2,
			_3,
			_4));
	const auto &connection = security.SubscribeToLevel1Ticks(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "level 1 ticks");
}

void SubscriptionsManager::SubscribeToTrades(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::NewTradeSlot(
		boost::bind(
			&Dispatcher::SignalNewTrade,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1,
			_2,
			_3,
			_4));
	const auto &connection = security.SubscribeToTrades(slot);
	try {
		slotConnections.emplace_back(connection);
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

void SubscriptionsManager::SubscribeToBrokerPositionUpdates(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::BrokerPositionUpdateSlot(
		boost::bind(
			&Dispatcher::SignalBrokerPositionUpdate,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1,
			_2));
	const auto &connection = security.SubscribeToBrokerPositionUpdates(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "broker position updates");
}

void SubscriptionsManager::SubscribeToBars(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::NewBarSlot(
		boost::bind(
			&Dispatcher::SignalNewBar,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1));
	const auto &connection = security.SubscribeToBars(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "new bars");
}

void SubscriptionsManager::SubscribeToBookUpdateTicks(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::BookUpdateTickSlot(
		boost::bind(
			&Dispatcher::SignalBookUpdateTick,
			&m_dispatcher,
			subscriber,
			boost::ref(security),
			_1,
			_2));
	const auto &connection = security.SubscribeToBookUpdateTicks(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "book update ticks");
}

void SubscriptionsManager::SubscribeToSecurityContractSwitching(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	
	typedef void(CallbackProto)(
		SubscriberPtrWrapper &,
		const pt::ptime &,
		const Security::Request &);
	const boost::function<CallbackProto> &callback = [this, &security](
			SubscriberPtrWrapper &subscriber,
			const pt::ptime &time,
			const Security::Request &request) {
		m_dispatcher.SignalSecurityContractSwitched(
			subscriber,
			time,
			security,
			// workaround for boost::bind "forwarding problem":
			const_cast<Security::Request &>(request));
	};

	const auto slot = Security::ContractSwitchingSlot(
		boost::bind(callback, subscriber, _1, _2));
	const auto &connection = security.SubscribeToContractSwitching(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}

	Report(*subscriber, security, "security contract switching");

}

void SubscriptionsManager::SubscribeToSecurityServiceEvents(
		Security &security,
		const SubscriberPtrWrapper &subscriber,
		std::list<sig::connection> &slotConnections) {
	const auto slot = Security::ServiceEventSlot(
		boost::bind(
			&Dispatcher::SignalSecurityServiceEvents,
			&m_dispatcher,
			subscriber,
			_1,
			boost::ref(security),
			_2));
	const auto &connection = security.SubscribeToServiceEvents(slot);
	try {
		slotConnections.emplace_back(connection);
	} catch (...) {
		try {
			connection.disconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
		throw;
	}
	Report(*subscriber, security, "security service events");
}

void SubscriptionsManager::SubscribeToLevel1Updates(
			Security &security,
			Strategy &strategy) {
	Assert(!IsActive());
	Subscribe(
		security,
		strategy,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Updates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1Updates(
			Security &security,
			Service &service) {
	Assert(!IsActive());
	Subscribe(
		security,
		service,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Updates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1Updates(
			Security &security,
			Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		security,
		observer,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Updates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1Ticks(
			Security &security,
			Strategy &strategy) {
	Assert(!IsActive());
	Subscribe(
		security,
		strategy,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Ticks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1Ticks(
			Security &security,
			Service &service) {
	Assert(!IsActive());
	Subscribe(
		security,
		service,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Ticks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToLevel1Ticks(
			Security &security,
			Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		security,
		observer,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToLevel1Ticks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(
			Security &security,
			Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToTrades(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(
			Security &security,
			Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToTrades(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(
			Security &security,
			Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		security,
		observer,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToTrades(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBrokerPositionUpdates(
			Security &security,
			Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBrokerPositionUpdates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBrokerPositionUpdates(
			Security &security,
			Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBrokerPositionUpdates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBrokerPositionUpdates(
			Security &security,
			Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		security,
		observer,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBrokerPositionUpdates(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBars(
			Security &security,
			Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBars(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBars(
			Security &security,
			Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBars(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBars(
			Security &security,
			Observer &observer) {
	Assert(!IsActive());
	Subscribe(
		security,
		observer,
		[this](
					Security &security,
					const SubscriberPtrWrapper &subscriber,
					std::list<sig::connection> &slotConnections) {
			SubscribeToBars(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBookUpdateTicks(
		Security &security,
		Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToBookUpdateTicks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBookUpdateTicks(
		Security &security,
		Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToBookUpdateTicks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToBookUpdateTicks(
		Security &security,
		Observer &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToBookUpdateTicks(security, subscriber, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityContractSwitching(
		Security &security,
		Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityContractSwitching(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityContractSwitching(
		Security &security,
		Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityContractSwitching(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityContractSwitching(
		Security &security,
		Observer &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityContractSwitching(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityServiceEvents(
		Security &security,
		Strategy &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityServiceEvents(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityServiceEvents(
		Security &security,
		Service &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityServiceEvents(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::SubscribeToSecurityServiceEvents(
		Security &security,
		Observer &subscriber) {
	Assert(!IsActive());
	Subscribe(
		security,
		subscriber,
		[this](
				Security &security,
				const SubscriberPtrWrapper &subscriber,
				std::list<sig::connection> &slotConnections) {
			SubscribeToSecurityServiceEvents(
				security,
				subscriber,
				slotConnections);
		});
}

void SubscriptionsManager::Subscribe(
		Security &security,
		Strategy &strategy,
		const SubscribeImpl &subscribeImpl) {

	if (
			m_subscribedStrategies.find(&strategy)
			!= m_subscribedStrategies.end()) {

		subscribeImpl(
			security,
			SubscriberPtrWrapper(strategy),
			m_slotConnections);

	} else {

		auto subscribedStrategies = m_subscribedStrategies;
		subscribedStrategies.insert(&strategy);
		auto slotConnections = m_slotConnections;
		const auto positionUpdateConnection
			= strategy.SubscribeToPositionsUpdates(
				boost::bind(
					&Dispatcher::SignalPositionUpdate,
					&m_dispatcher,
					SubscriberPtrWrapper(strategy),
					_1));

		try {
			slotConnections.emplace_back(positionUpdateConnection);
			subscribeImpl(
				security,
				SubscriberPtrWrapper(strategy),
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
			Security &security,
			Service &service,
			const SubscribeImpl &subscribeImpl) {

	struct ServiceSubscriberVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {

	public:

		explicit ServiceSubscriberVisitor(
					Dispatcher &dispatcher,
					std::list<sig::connection> &connections,
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
				m_connections.emplace_back(positionUpdateConnection);
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
		std::list<sig::connection> &m_connections;
		std::set<const Strategy *> &m_subscribed;

	};

	decltype(m_slotConnections) newSlotConnections;
	auto slotConnections = m_slotConnections;
	auto subscribedStrategies = m_subscribedStrategies;
	
	ServiceSubscriberVisitor serviceSubscriberVisitor(
		m_dispatcher,
		newSlotConnections,
		subscribedStrategies);
	try {
		foreach (const auto &serviceSubscriber, service.GetSubscribers()) {
			boost::apply_visitor(serviceSubscriberVisitor, serviceSubscriber);
		}
		subscribeImpl(
			security,
			SubscriberPtrWrapper(service),
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
			Security &security,
			Observer &observer,
			const SubscribeImpl &subscribeImpl) {
	subscribeImpl(
		security,
		SubscriberPtrWrapper(observer),
		m_slotConnections);
}

////////////////////////////////////////////////////////////////////////////////
