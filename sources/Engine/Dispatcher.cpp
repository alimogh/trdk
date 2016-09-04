/**************************************************************************
 *   Created: 2012/11/22 11:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Context.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	struct NoMeasurementPolicy {
		static TimeMeasurement::Milestones StartDispatchingTimeMeasurement(
				const trdk::Context &) {
			return TimeMeasurement::Milestones();
		}
	};

	struct DispatchingTimeMeasurementPolicy {
		static TimeMeasurement::Milestones StartDispatchingTimeMeasurement(
				const trdk::Context &context) {
			return context.StartDispatchingTimeMeasurement();
		}
	};

}

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher(Engine::Context &context)
	: m_context(context)
	, m_level1Updates("Level 1 updates", m_context)
	, m_level1Ticks("Level 1 ticks", m_context)
	, m_newTrades("Trades", m_context)
	, m_positionsUpdates("Positions", m_context)
	, m_brokerPositionsUpdates("Broker positions", m_context)
	, m_newBars("Bars", m_context)
	, m_bookUpdateTicks("Book update ticks", m_context)
	, m_securityServiceEvents("Security service events", m_context) {
	unsigned int threadsCount = 1;
	boost::shared_ptr<boost::barrier> startBarrier(
		new boost::barrier(threadsCount + 1));
	// @sa Dispatcher::~Dispatcher
	// @sa Dispatcher::Activate
	// @sa Dispatcher::Suspend
	// @sa Dispatcher::IsActive
	StartNotificationTask<DispatchingTimeMeasurementPolicy>(
		startBarrier,
		m_level1Updates,
		m_level1Ticks,
		m_bookUpdateTicks,
		m_newTrades,
		m_positionsUpdates,
		m_newBars,
		m_brokerPositionsUpdates,
		m_securityServiceEvents,
		threadsCount);
	AssertEq(0, threadsCount);
	startBarrier->wait();
}

Dispatcher::~Dispatcher() {
	try {
		m_context.GetLog().Debug("Stopping events dispatching...");
		// @sa Dispatcher::Dispatcher
		// @sa Dispatcher::Activate
		// @sa Dispatcher::Suspend
		// @sa Dispatcher::IsActive
		m_level1Updates.Stop();
		m_level1Ticks.Stop();
		m_bookUpdateTicks.Stop();
		m_newTrades.Stop();
		m_positionsUpdates.Stop();
		m_newBars.Stop();
		m_brokerPositionsUpdates.Stop();
		m_securityServiceEvents.Stop();
		m_threads.join_all();
		m_context.GetLog().Debug("Events dispatching stopped.");
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

void Dispatcher::Activate() {
	m_context.GetLog().Debug("Starting events dispatching...");
	// @sa Dispatcher::Dispatcher
	// @sa Dispatcher::~Dispatcher
	// @sa Dispatcher::Suspend
	// @sa Dispatcher::IsActive
	m_level1Updates.Activate();
	m_level1Ticks.Activate();
	m_bookUpdateTicks.Activate();
	m_newTrades.Activate();
	m_positionsUpdates.Activate();
	m_newBars.Activate();
	m_brokerPositionsUpdates.Activate();
	m_securityServiceEvents.Activate();
	m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
	m_context.GetLog().Debug("Suspending events dispatching...");
	// @sa Dispatcher::Disptcher
	// @sa Dispatcher::~Dispatcher
	// @sa Dispatcher::Activate
	// @sa Dispatcher::IsActive
	m_level1Updates.Suspend();
	m_level1Ticks.Suspend();
	m_bookUpdateTicks.Suspend();
	m_newTrades.Suspend();
	m_positionsUpdates.Suspend();
	m_newBars.Suspend();
	m_brokerPositionsUpdates.Suspend();
	m_securityServiceEvents.Suspend();
	m_context.GetLog().Debug("Events dispatching suspended.");
}

bool Dispatcher::IsActive() const {
	// @sa Dispatcher::Disptcher
	// @sa Dispatcher::~Dispatcher
	// @sa Dispatcher::Activate
	// @sa Dispatcher::Suspend
	return
		m_level1Updates.IsActive()
		|| m_level1Ticks.IsActive()
		|| m_bookUpdateTicks.IsActive()
		|| m_newTrades.IsActive()
		|| m_positionsUpdates.IsActive()
		|| m_newBars.IsActive()
		|| m_brokerPositionsUpdates.IsActive()
		|| m_securityServiceEvents.IsActive();
}

void Dispatcher::SignalLevel1Update(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const TimeMeasurement::Milestones &timeMeasurement) {
	if (subscriber.IsBlocked()) {
		return;
	}
	m_level1Updates.Queue(
		boost::make_tuple(&security, subscriber, timeMeasurement),
		true);
}

void Dispatcher::SignalLevel1Tick(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const boost::posix_time::ptime &time,
		const Level1TickValue &value,
		bool flush) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		const SubscriberPtrWrapper::Level1Tick tick(security, time, value);
		m_level1Ticks.Queue(
			boost::make_tuple(tick, subscriber),
			flush);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalNewTrade(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const pt::ptime &time,
		const ScaledPrice &price,
		const Qty &qty,
		const OrderSide &side) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		const SubscriberPtrWrapper::Trade trade = {
			&security,
			time,
			price,
			qty,
			side};
		m_newTrades.Queue(boost::make_tuple(trade, subscriber), true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalPositionUpdate(
		SubscriberPtrWrapper &subscriber,
		Position &position) {
	try {
		m_positionsUpdates.Queue(
			boost::make_tuple(position.shared_from_this(), subscriber),
			true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalBrokerPositionUpdate(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const Qty &qty,
		bool isInitial) {
	try {
		const SubscriberPtrWrapper::BrokerPosition position = {
			&security,
			qty,
			isInitial
		};
		m_brokerPositionsUpdates.Queue(
			boost::make_tuple(position , subscriber),
			true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalNewBar(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const Security::Bar &bar) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		m_newBars.Queue(boost::make_tuple(&security, bar, subscriber), true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalBookUpdateTick(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const PriceBook &book,
		const TimeMeasurement::Milestones &timeMeasurement) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		m_bookUpdateTicks.Queue(
			boost::make_tuple(
				&security,
				book,
				timeMeasurement,
				subscriber),
			true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

void Dispatcher::SignalSecurityServiceEvents(
		SubscriberPtrWrapper &subscriber,
		Security &security,
		const Security::ServiceEvent &serviceEvent) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		m_securityServiceEvents.Queue(
			boost::make_tuple(&security, serviceEvent, subscriber),
			true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

//////////////////////////////////////////////////////////////////////////
