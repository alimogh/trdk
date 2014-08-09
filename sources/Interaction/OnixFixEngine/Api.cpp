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
#include "Core/TradeSystem.hpp"
#include "Core/MarketDataSource.hpp"

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_ONIXFIXENGINE_API
#else
#	define TRDK_INTERACTION_ONIXFIXENGINE_API extern "C"
#endif

using namespace trdk;
using namespace trdk::Lib;

TRDK_INTERACTION_ONIXFIXENGINE_API
TradeSystemFactoryResult CreateTradeSystem(
			const IniSectionRef &/*configuration*/,
			Context::Log &/*log*/) {
	TradeSystemFactoryResult result;
	boost::get<0>(result);
	boost::get<1>(result);
	return result;
}

TRDK_INTERACTION_ONIXFIXENGINE_API
boost::shared_ptr<MarketDataSource> CreateMarketDataSource(
			const IniSectionRef &/*configuration*/,
			Context::Log &/*log*/) {
	return boost::shared_ptr<MarketDataSource>();
}
