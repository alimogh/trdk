/**************************************************************************
 *   Created: 2012/11/22 11:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

Dispatcher::Dispatcher(boost::shared_ptr<const Settings> &settings)
			: m_level1Updates("Level 1", settings),
			m_newTrades("New Trades", settings),
			m_positionUpdates("Position", settings) {
	StartNotificationTask(m_level1Updates);
	StartNotificationTask(m_newTrades);
	StartNotificationTask(m_positionUpdates);
}

Dispatcher::~Dispatcher() {
	try {
		Log::Debug("Stopping events dispatching...");
		m_newTrades.Stop();
		m_level1Updates.Stop();
		m_positionUpdates.Stop();
		m_threads.join_all();
		Log::Debug("Events dispatching stopped.");
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void Dispatcher::SignalLevel1Update(
			boost::shared_ptr<Notifier> &notifier,
			const Security &security) {
	if (notifier->IsBlocked()) {
		return;
	}
	m_level1Updates.Queue(boost::make_tuple(&security, notifier));
}

void Dispatcher::SignalNewTrade(
			boost::shared_ptr<Notifier> &notifier,
			const Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	try {
		if (notifier->IsBlocked()) {
			return;
		}
		boost::shared_ptr<Notifier::Trade> trade(new Notifier::Trade);
		trade->security = &security;
		trade->time = time;
		trade->price = price;
		trade->qty = qty;
		trade->side = side;
		m_newTrades.Queue(boost::make_tuple(trade, notifier));
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		notifier->Block();
		throw;
	}
}

void Dispatcher::SignalPositionUpdate(
			boost::shared_ptr<Notifier> &notifier,
			Position &position) {
	try {
		m_positionUpdates.Queue(
			boost::make_tuple(position.shared_from_this(), notifier));
	} catch (...) {
		//! Blocking as irreversible error, data loss.
		notifier->Block();
		throw;
	}
}
