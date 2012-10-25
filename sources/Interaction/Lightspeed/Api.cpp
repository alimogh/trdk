/**************************************************************************
 *   Created: 2012/08/28 01:40:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Gateway.hpp"

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::TradeSystem> CreateTradeSystem() {
		return boost::shared_ptr<Trader::TradeSystem>(
			new Trader::Interaction::Lightspeed::Gateway);
	}
#else
	extern "C" boost::shared_ptr<Trader::TradeSystem> CreateTradeSystem() {
		return boost::shared_ptr<Trader::TradeSystem>(
			new Trader::Interaction::Lightspeed::Gateway);
	}
#endif
