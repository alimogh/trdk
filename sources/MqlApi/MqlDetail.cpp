/**************************************************************************
 *   Created: 2014/01/15 23:15:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MqlDetail.hpp"
#include "Core/Context.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;
using namespace trdk::MqlApi::Detail;

Symbol Detail::GetSymbol(Context &context, std::string symbol) {
	boost::erase_all(symbol, ":");
	return Symbol::ParseCash(
		Symbol::SECURITY_TYPE_FUTURE_OPTION,
		symbol,
		context.GetSettings().GetDefaultExchange());
}

Security & Detail::GetSecurity(Context &context, const std::string &symbol) {
	return context.GetSecurity(GetSymbol(context, symbol));
}
