/**************************************************************************
 *   Created: 2012/11/22 11:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Context.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

Dispatcher::Dispatcher(Engine::Context &context)
			: m_context(context),
			m_level1Updates("Level 1", m_context),
			m_newTrades("New Trades", m_context),
			m_positionUpdates("Position", m_context) {
	StartNotificationTask(m_level1Updates);
	StartNotificationTask(m_newTrades);
	StartNotificationTask(m_positionUpdates);
}

Dispatcher::~Dispatcher() {
	try {
		m_context.GetLog().Debug("Stopping events dispatching...");
		m_newTrades.Stop();
		m_level1Updates.Stop();
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
	m_level1Updates.Activate();
	m_newTrades.Activate();
	m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
	m_context.GetLog().Debug("Suspending events dispatching...");
	m_newTrades.Suspend();
	m_level1Updates.Suspend();
	m_positionUpdates.Suspend();
	m_context.GetLog().Debug("Events dispatching suspended.");
}

void Dispatcher::SignalLevel1Update(
			SubscriberPtrWrapper &subscriber,
			const Security &security) {
	if (subscriber.IsBlocked()) {
		return;
	}
	m_level1Updates.Queue(boost::make_tuple(&security, subscriber));
}

void Dispatcher::SignalNewTrade(
			SubscriberPtrWrapper &subscriber,
			const Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	try {
		if (subscriber.IsBlocked()) {
			return;
		}
		boost::shared_ptr<SubscriberPtrWrapper::Trade> trade(
			new SubscriberPtrWrapper::Trade);
		trade->security = &security;
		trade->time = time;
		trade->price = price;
		trade->qty = qty;
		trade->side = side;
		m_newTrades.Queue(boost::make_tuple(trade, subscriber));
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
		m_positionUpdates.Queue(
			boost::make_tuple(position.shared_from_this(), subscriber));
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		subscriber.Block();
		throw;
	}
}

//////////////////////////////////////////////////////////////////////////
