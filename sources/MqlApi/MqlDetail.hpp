/**************************************************************************
 *   Created: 2014/01/15 23:04:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace MqlApi { namespace Detail {

	Lib::Symbol GetSymbol(Context &, std::string symbol);
	Security & GetSecurity(Context &, const std::string &symbol);

} } }
