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
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

Dispatcher::Dispatcher(Engine::Context &context)
			: m_context(context),
			m_level1Ticks("Level 1 Ticks", m_context),
			m_positionUpdates("Position", m_context) {
	StartNotificationTask(m_level1Ticks, m_positionUpdates);
}

Dispatcher::~Dispatcher() {
	try {
		m_context.GetLog().Debug("Stopping events dispatching...");
		m_level1Ticks.Stop();
		m_positionUpdates.Stop();
		m_threads.join_all();
		m_context.GetLog().Debug("Events dispatching stopped.");
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void Dispatcher::Activate() {
	m_context.GetLog().Debug("Starting events dispatching...");
	m_positionUpdates.Activate();
	m_level1Ticks.Activate();
	m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
	m_context.GetLog().Debug("Suspending events dispatching...");
	m_level1Ticks.Suspend();
	m_positionUpdates.Suspend();
	m_context.GetLog().Debug("Events dispatching suspended.");
}

void Dispatcher::SignalLevel1Update(
			SubscriberPtrWrapper &,
			Security &) {
	AssertFail("Not implemented!");
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
			SubscriberPtrWrapper &,
			Security &,
			const pt::ptime &,
			ScaledPrice,
			Qty,
			OrderSide) {
	AssertFail("Not implemented!");
}

void Dispatcher::SignalPositionUpdate(
			SubscriberPtrWrapper &subscriber,
			Position &position) {
	try {
		m_positionUpdates.Queue(
			boost::make_tuple(position.shared_from_this(), subscriber), true);
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

//////////////////////////////////////////////////////////////////////////
