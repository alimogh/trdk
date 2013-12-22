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
			m_positionsUpdates("Positions", m_context),
			m_brokerPositionsUpdates("Broker Positions", m_context) {
	size_t threadsCount = 1;
	boost::barrier startBarrier(threadsCount + 1);
	StartNotificationTask(
		startBarrier,
		m_positionsUpdates,
		m_brokerPositionsUpdates,
		threadsCount);
	AssertEq(0, threadsCount);
	startBarrier.wait();
}

Dispatcher::~Dispatcher() {
	try {
		m_context.GetLog().Debug("Stopping events dispatching...");
		m_positionsUpdates.Stop();
		m_brokerPositionsUpdates.Stop();
		m_threads.join_all();
		m_context.GetLog().Debug("Events dispatching stopped.");
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void Dispatcher::Activate() {
	m_context.GetLog().Debug("Starting events dispatching...");
	m_brokerPositionsUpdates.Activate();
	m_positionsUpdates.Activate();
	m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
	m_context.GetLog().Debug("Suspending events dispatching...");
	m_positionsUpdates.Suspend();
	m_context.GetLog().Debug("Events dispatching suspended.");
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
			Qty qty,
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

//////////////////////////////////////////////////////////////////////////
