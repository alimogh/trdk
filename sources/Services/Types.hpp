/**************************************************************************
 *   Created: 2014/09/14 22:49:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Services {

	//! Moving Average Value data point.
	struct MovingAveragePoint {
		ScaledPrice source;
		double value;
	};


} }
