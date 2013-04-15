/**************************************************************************
 *   Created: 2012/07/09 00:29:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRADER_CORE_API PositionReporter : private boost::noncopyable {

	public:

		PositionReporter();
		virtual ~PositionReporter();

	public:

		virtual void ReportClosedPositon(const trdk::Position &) = 0;

	};

}
