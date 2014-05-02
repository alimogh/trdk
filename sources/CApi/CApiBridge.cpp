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

namespace {

	Symbol::Right ParseFutOptRight(const std::string &source) {
		if (boost::iequals(source, "put")) {
			return Symbol::RIGHT_PUT;
		} else if (boost::iequals(source, "call")) {
			return Symbol::RIGHT_CALL;
		} else {
			throw Exception("Failed to resolve right (Put or Call)");
		}
	}

}

Bridge::Bridge(boost::shared_ptr<Context> context)
		: m_context(context) {
	//...//
}

Bridge::SecurityId Bridge::ResolveFutOpt(
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
				ParseFutOptRight(right),
 				exchange));
	return SecurityId(&security);
}

Security & Bridge::GetSecurity(const SecurityId &id) {
	return *reinterpret_cast<Security *>(id);
}

const Security & Bridge::GetSecurity(const SecurityId &id) const {
	return const_cast<Bridge *>(this)->GetSecurity(id);
}

double Bridge::GetCashBalance() const {
	return m_context->GetTradeSystem().GetAccount().cashBalance;
}
