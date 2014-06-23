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
#include "SimpleApiBridge.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

Bridge::Bridge(boost::shared_ptr<Context> context)
		: m_context(context) {
	//...//
}

Bridge::SecurityHandle Bridge::ResolveFutOpt(
			const std::string &symbol,
			const std::string &exchange,
			const std::string &expirationDate,
			double strike,
			const std::string &right)
		const {
	const Security &security
		= m_context->GetSecurity(
			Symbol::ParseCashFutureOption(
 				boost::erase_all_copy(symbol, ":"), // TRDK-reserver delimiter
 				expirationDate,
 				strike,
				Symbol::ParseRight(right),
 				exchange));
	return SecurityHandle(&security);
}

Security & Bridge::GetSecurity(const SecurityHandle &id) {
	return *reinterpret_cast<Security *>(id);
}

const Security & Bridge::GetSecurity(const SecurityHandle &id) const {
	return const_cast<Bridge *>(this)->GetSecurity(id);
}

double Bridge::GetCashBalance() const {
	return m_context->GetTradeSystem().GetAccount().cashBalance;
}
