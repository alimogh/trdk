/**************************************************************************
 *   Created: 2012/11/04 12:33:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace trdk {

	class TRADER_CORE_API SecurityAlgo : public trdk::Module {

	public:

		explicit SecurityAlgo(
				trdk::Context &,
				const std::string &typeName,
				const std::string &name,
				const std::string &tag);
		virtual ~SecurityAlgo();

	public:

		virtual const trdk::Security & GetSecurity() const = 0;

	};

}

