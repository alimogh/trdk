/**************************************************************************
 *   Created: 2012/08/07 16:15:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace PyApi {

	enum MarketDataSource {
		MARKET_DATA_SOURCE_NOT_SET				= 0,
		MARKET_DATA_SOURCE_DISABLED,
		MARKET_DATA_SOURCE_IQFEED,
		MARKET_DATA_SOURCE_INTERACTIVE_BROKERS
	};

}
