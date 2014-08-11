/**************************************************************************
 *   Created: 2014/08/09 01:17:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CurrenexFixExchange.hpp"

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_ONIXFIXENGINE_API
#else
#	define TRDK_INTERACTION_ONIXFIXENGINE_API extern "C"
#endif

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Onyx;

TRDK_INTERACTION_ONIXFIXENGINE_API
TradeSystemFactoryResult CreateCurrenexFixExchange(
			const IniSectionRef &configuration,
			Context::Log &log) {
	boost::shared_ptr<CurrenexFixExchange> tradeSystem(
		new CurrenexFixExchange(configuration, log));
	TradeSystemFactoryResult result;
	boost::get<0>(result) = tradeSystem;
	boost::get<1>(result) = tradeSystem;
	return result;
}
