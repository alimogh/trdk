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
				true,
				SupportedLevel1Types().set()) {
			StartLevel1();
		}

	public:

		using Security::SetBook;

	};

} } }
