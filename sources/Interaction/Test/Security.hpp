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

namespace trdk { namespace Interaction { namespace Test {

	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit Security(
				Context &context,
				const Lib::Symbol &symbol,
				MarketDataSource &source)
			: Base(
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

		using Base::SetOnline;
		using Base::SetTradingSessionState;
		using Base::SetExpiration;
		using Base::SetBook;
		using Base::AddTrade;
		using trdk::Security::SetLevel1;

	};

} } }
