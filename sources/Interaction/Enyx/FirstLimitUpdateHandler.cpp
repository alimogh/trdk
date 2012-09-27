/**************************************************************************
 *   Created: 2012/09/23 10:12:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "FirstLimitUpdateHandler.hpp"

using namespace Trader::Interaction::Enyx;

FirstLimitUpdateHandler::FirstLimitUpdateHandler() {
	//...//
}

FirstLimitUpdateHandler::~FirstLimitUpdateHandler() {
	//...//
}

void FirstLimitUpdateHandler::Register(
			const boost::shared_ptr<Security> &security)
		throw() {
	try {
		SecurityHolder holder;
		holder.security = security;
		m_securities.insert(holder);
	} catch (...) {
		AssertFailNoException();
	}
}

void FirstLimitUpdateHandler::HandleUpdate(
			const std::string &symbol,
			Security::ScaledPrice price,
			Security::Qty qty,
			bool isBuy) {
	const SecurityList::const_iterator pos = m_securities.find(symbol);
	if (pos == m_securities.end()) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX
				"failed to find symbols for First Limit Update"
				" (symbol: %1%, price: %2%, qty: %3%, is buy: %4%).",
			symbol,
			price,
			qty,
			isBuy);
		return;
	}
	pos->security->SignaleNewTrade(boost::get_system_time(), isBuy, price, qty);
}
