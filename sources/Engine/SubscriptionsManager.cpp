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

namespace mi = boost::multi_index;
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
	slotConnections.push_back(security.SubcribeToLevel1(slot));
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
	slotConnections.push_back(security.SubcribeToTrades(slot));
	Report(notifier->GetObserver(), security, "new trades");
}

void SubscriptionsManager::SubscribeToLevel1(Strategy &strategy) {
	SubscribeToLevel1(
		boost::shared_ptr<Notifier>(new Notifier(strategy.shared_from_this())),
		strategy.GetSecurity(),
		m_slotConnections);
}

void SubscriptionsManager::SubscribeToLevel1(Service &service) {
	SubscribeToLevel1(
		boost::shared_ptr<Notifier>(new Notifier(service.shared_from_this())),
		service.GetSecurity(),
		m_slotConnections);
}

void SubscriptionsManager::SubscribeToLevel1(Observer &observer) {
	SubscribeToTrades(
		observer,
		[this](
				boost::shared_ptr<Notifier> notifier,
				const Security &security,
				std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToLevel1(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Strategy &observer) {
	SubscribeToTrades(
		observer,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(Service &observer) {
	SubscribeToTrades(
		boost::shared_ptr<Notifier>(new Notifier(observer.shared_from_this())),
		observer.GetSecurity(),
		m_slotConnections);
}

void SubscriptionsManager::SubscribeToTrades(Observer &observer) {
	SubscribeToTrades(
		observer,
		[this](
					boost::shared_ptr<Notifier> notifier,
					const Security &security,
					std::list<boost::signals2::connection> &slotConnections) {
			SubscribeToTrades(notifier, security, slotConnections);
		});
}

void SubscriptionsManager::SubscribeToTrades(
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
		const auto positionUpdateSignal = observer.SubscribeToPositionsUpdates(
			boost::bind(
				&Dispatcher::SignalPositionUpdate,
				&m_dispatcher,
				notifier,
				_1));
		slotConnections.push_back(positionUpdateSignal);
		try {
			subscribeImpl(notifier, observer.GetSecurity(), slotConnections);
		} catch (...) {
			try {
				positionUpdateSignal.disconnect();
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

void SubscriptionsManager::SubscribeToTrades(
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
