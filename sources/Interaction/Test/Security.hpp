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
				MarketDataSource &source,
				const SupportedLevel1Types &supportedData)
			: Base(context, symbol,  source, supportedData) {
			//...//
		}

	public:

		using Base::SetOnline;
		using Base::SetTradingSessionState;
		using Base::SetExpiration;
		using Base::SetBook;
		using Base::AddTrade;
		using Base::SetLevel1;

	};

} } }
