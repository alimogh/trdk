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
#include "Services/BarService.hpp"
#include "Engine/Context.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;
using namespace trdk::Services;



Bridge::Bridge(boost::shared_ptr<Engine::Context> context)
		: m_context(context) {
	//...//
}

Security & Bridge::ResolveOpt(
			const std::string &symbolStr,
			const std::string &exchange,
			double expirationDate,
			double strike,
			const std::string &right,
			const std::string &tradingClass,
			const std::string &type)
		const {
	const Symbol &symbol = Symbol::ParseCashOption(
 		boost::erase_all_copy(symbolStr, ":"), // TRDK-reserver delimiter
		boost::lexical_cast<std::string>(size_t(expirationDate)),
 		strike,
		Symbol::ParseRight(right),
		tradingClass,
 		exchange,
		type);
	return m_context->GetSecurity(symbol);
}

double Bridge::GetCashBalance() const {
	return m_context->GetTradeSystem().GetAccount().cashBalance;
}

extern boost::atomic_bool isIbActive;

bool Bridge::CheckActive() const {
	return isIbActive.exchange(true);
}
