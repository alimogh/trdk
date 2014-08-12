/**************************************************************************
 *   Created: 2014/08/12 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace Onyx {

	class CurrenexSecurity : public trdk::Security {

	public:

		explicit CurrenexSecurity(Context &context, const Lib::Symbol &symbol)
				: Security(context, symbol) {
			//...//
		}

	};

} } }
