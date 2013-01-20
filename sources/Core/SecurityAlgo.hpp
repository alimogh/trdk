/**************************************************************************
 *   Created: 2012/11/04 12:33:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API SecurityAlgo : public Trader::Module {

	public:

		explicit SecurityAlgo(
				const std::string &typeName,
				const std::string &name,
				const std::string &tag,
				boost::shared_ptr<const Settings>);
		virtual ~SecurityAlgo();

	public:

		virtual const Trader::Security & GetSecurity() const = 0;

	protected:

		boost::posix_time::ptime GetCurrentTime() const;

	};

}

