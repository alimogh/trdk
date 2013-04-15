/**************************************************************************
 *   Created: 2012/11/18 11:09:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"

namespace trdk {

	typedef boost::uint64_t OrderId;

	enum OrderSide {
		ORDER_SIDE_BUY,
		ORDER_SIDE_SELL,
		numberOfOrderSides
	};
	
	typedef boost::int32_t Qty;
	
	typedef boost::int64_t ScaledPrice;

}
