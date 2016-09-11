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
using namespace trdk::Engine::Details;

////////////////////////////////////////////////////////////////////////////////

namespace {

	////////////////////////////////////////////////////////////////////////////////
	
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

	////////////////////////////////////////////////////////////////////////////////

	struct StopVisitor : public boost::static_visitor<> {
		template<typename T>
		void operator ()(T *obj) const {
			obj->Stop();
		}
	};

	struct ActivateVisitor : public boost::static_visitor<> {
		template<typename T>
		void operator ()(T *obj) const {
			obj->Activate();
		}
	};

	struct SuspendVisitor : public boost::static_visitor<> {
		template<typename T>
		void operator ()(T *obj) const {
			obj->Suspend();
		}
	};

	struct IsActiveVisitor : public boost::static_visitor<bool> {
		template<typename T>
		bool operator ()(const T *obj) const {
			return obj->IsActive();
		}
	};

	struct SyncVisitor : public boost::static_visitor<> {
		template<typename T>
		void operator ()(T *obj) const {
			obj->Sync();
		}
	};

	////////////////////////////////////////////////////////////////////////////////

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

	m_queues.emplace_back(&m_level1Updates);
	m_queues.emplace_back(&m_level1Ticks);
	m_queues.emplace_back(&m_newTrades);
	m_queues.emplace_back(&m_positionsUpdates);
	m_queues.emplace_back(&m_brokerPositionsUpdates);
	m_queues.emplace_back(&m_newBars);
	m_queues.emplace_back(&m_bookUpdateTicks);
	m_queues.emplace_back(&m_securityServiceEvents);
	m_queues.shrink_to_fit();

	unsigned int threadsCount = 1;
	
	// Working in the multi-threading mode is not supported by service events.
	// Such service should: 1) stop all notifications (but stop events
	// queueing); 2) wait untill all queued events will be flushed; 3) make
	// service event notification; 4) start notifications again.
	// @sa SignalSecurityServiceEvents
	AssertEq(1, threadsCount);

	boost::shared_ptr<boost::barrier> startBarrier(
		new boost::barrier(threadsCount + 1));
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
		for (auto &queue: m_queues) {
			boost::apply_visitor(StopVisitor(), queue);
		}
		m_threads.join_all();
		m_context.GetLog().Debug("Events dispatching stopped.");
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

void Dispatcher::Activate() {
	m_context.GetLog().Debug("Starting events dispatching...");
	for (auto &queue: m_queues) {
		boost::apply_visitor(ActivateVisitor(), queue);
	}
	m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
	m_context.GetLog().Debug("Suspending events dispatching...");
	for (auto &queue: m_queues) {
		boost::apply_visitor(SuspendVisitor(), queue);
	}
	m_context.GetLog().Debug("Events dispatching suspended.");
}

void Dispatcher::SyncDispatching() {
	for (auto &queue: m_queues) {
		boost::apply_visitor(SyncVisitor(), queue);
	}
}

bool Dispatcher::IsActive() const {
	for (auto &queue: m_queues) {
		if (!boost::apply_visitor(IsActiveVisitor(), queue)) {
			return false;
		}
	}
	return true;
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
		const Qty &qty) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		const SubscriberPtrWrapper::Trade trade = {
			&security,
			time,
			price,
			qty};
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
		const Security::ServiceEvent &serviceEvent,
		bool flush) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		SyncDispatching();
		m_securityServiceEvents.Queue(
			boost::make_tuple(&security, serviceEvent, subscriber),
			flush);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

//////////////////////////////////////////////////////////////////////////
