/**************************************************************************
 *   Created: 2016/11/21 10:12:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace IqFeed {

	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;
	
	public:

		explicit Security(
				Context &,
				const Lib::Symbol &,
				trdk::MarketDataSource &);

	public:
		
		using Base::SetOnline;
		using Base::SetTradingSessionState;
		using Base::SwitchTradingSession;
		using Base::SetExpiration;
		using Base::IsLevel1Required;
		using Base::SetLevel1;
		using Base::AddTrade;
		using Base::AddBar;

	};

} } }
