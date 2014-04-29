/**************************************************************************
 *   Created: 2014/04/29 23:28:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CApiBridge.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::CApi;

Bridge::Bridge(
			Context &context,
			const std::string &account,
			const std::string &expirationDate,
			double strike)
		: m_context(context),
		m_account(account),
		m_expirationDate(expirationDate),
		m_strike(strike) {
	//...//
}

Symbol Bridge::GetSymbol(std::string symbol) const {
	boost::erase_all(symbol, ":");
	return Symbol::ParseCashFutureOption(
 		symbol,
 		m_expirationDate,
 		m_strike,
 		m_context.GetSettings().GetDefaultExchange());
}

Security & Bridge::GetSecurity(const std::string &symbol) const {
	return m_context.GetSecurity(GetSymbol(symbol));
}

Qty Bridge::GetPosition(const std::string &symbol) const {
	const auto &pos
		= m_context
			.GetTradeSystem()
			.GetBrokerPostion(m_account, GetSymbol(symbol));
	return pos.qty;
}

double Bridge::GetCashBalance() const {
	return m_context.GetTradeSystem().GetAccount().cashBalance;
}
