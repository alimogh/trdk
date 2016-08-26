/**************************************************************************
 *   Created: 2016/08/25 18:06:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class Security : public trdk::Security {
	
	public:

		explicit Security(
				Context &context,
				const Lib::Symbol &symbol,
				const MarketDataSource &source)
			: trdk::Security(context, symbol, source, true) {
			//...//
		}
	
	};

} } }
