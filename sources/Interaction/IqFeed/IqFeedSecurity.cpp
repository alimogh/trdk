/**************************************************************************
 *   Created: 2016/11/21 10:15:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IqFeedSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::IqFeed;

IqFeed::Security::Security(
		Context &context,
		const Symbol &symbol,
		trdk::MarketDataSource &source)
	: Base(
		context,
		symbol,
		source,
		SupportedLevel1Types()
			.set(LEVEL1_TICK_LAST_PRICE)
			.set(LEVEL1_TICK_BID_PRICE)
			.set(LEVEL1_TICK_BID_QTY)
			.set(LEVEL1_TICK_ASK_PRICE)
			.set(LEVEL1_TICK_ASK_QTY)) {
	//...//
}
