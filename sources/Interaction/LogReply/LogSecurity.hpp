/**************************************************************************
 *   Created: 2014/11/29 14:09:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {  namespace Interaction { namespace LogReply {

	class LogSecurity : public Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit LogSecurity(
					Context &context,
					const Lib::Symbol &symbol,
					const trdk::MarketDataSource &source)
				: Base(context, symbol, source) {
			StartLevel1();
		}

	public:

		using Base::StartBookUpdate;

	};

} } }
