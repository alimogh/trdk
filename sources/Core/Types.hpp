/**************************************************************************
 *   Created: 2012/11/18 11:09:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

namespace Trader {

	typedef boost::uint64_t OrderId;

	enum OrderSide {
		ORDER_SIDE_BUY,
		ORDER_SIDE_SELL,
		numberOfOrderSides
	};
	
	typedef boost::int32_t Qty;
	
	typedef boost::int64_t ScaledPrice;

}
