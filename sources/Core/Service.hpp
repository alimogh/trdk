/**************************************************************************
 *   Created: 2012/11/03 00:19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgo.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Service
			: public Trader::SecurityAlgo,
			public boost::enable_shared_from_this<Trader::Service> {

	public:

		explicit Service(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>);
		virtual ~Service();

	};

}
