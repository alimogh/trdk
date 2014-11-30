/**************************************************************************
 *   Created: 2014/11/29 22:53:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace trdk { namespace Interaction { namespace LogReply { namespace Detail {

	inline boost::filesystem::path GetSecurityFilename(
			const MarketDataSource &source,
			const Lib::Symbol &symbol) {
		return
			source.GetTag()
				+ '_'
				+ boost::erase_all_copy(symbol.GetSymbol(), "/");
	}

} } } }
