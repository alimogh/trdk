/**************************************************************************
 *   Created: 2016/11/07 03:23:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Strategies { namespace GadM {

	//! Trailing stop settings.
	/** @sa https://app.asana.com/0/186349222941752/196887742980415
	  */
	struct TrailingStop {
		double profitToActivate;
		double minProfit;
	};

} } } 