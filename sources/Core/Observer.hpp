/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Observer
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Observer> {

	public:

		Observer();
		virtual ~Observer();

	};

}
