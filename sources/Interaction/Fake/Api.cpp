/**************************************************************************
 *   Created: 2012/09/15 23:34:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FakeTradeSystem.hpp"
#include "FakeMarketDataSource.hpp"

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_FAKE_API
#else
#	define TRDK_INTERACTION_FAKE_API extern "C"
#endif

TRDK_INTERACTION_FAKE_API
trdk::TradeSystemFactoryResult CreateTradeSystem(
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	trdk::TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new trdk::Interaction::Fake::TradeSystem(
			index,
			context,
			tag,
			configuration));
	return result;
}

TRDK_INTERACTION_FAKE_API
boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new trdk::Interaction::Fake::MarketDataSource(
			index,
			context,
			tag,
			configuration));
}
