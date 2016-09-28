/*******************************************************************************
 *   Created: 2016/01/09 12:42:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace Fake {

	class FakeSecurity : public Security {

	public:

		explicit FakeSecurity(
				Context &context,
				const Lib::Symbol &symbol,
				MarketDataSource &source)
			: Security(
				context,
				symbol, 
				source,
				SupportedLevel1Types()
					.set(LEVEL1_TICK_BID_PRICE)
					.set(LEVEL1_TICK_BID_QTY)
					.set(LEVEL1_TICK_ASK_PRICE)
					.set(LEVEL1_TICK_ASK_QTY)
					.set(LEVEL1_TICK_LAST_PRICE)
					.set(LEVEL1_TICK_LAST_QTY)
					.set(LEVEL1_TICK_TRADING_VOLUME)) {
			//...//
		}

	public:

		using Security::SetOnline;
		using Security::SetTradingSessionState;
		using Security::SetExpiration;
		using Security::SetBook;
		using Security::AddTrade;
		using Security::SetLevel1;

	};

} } }
