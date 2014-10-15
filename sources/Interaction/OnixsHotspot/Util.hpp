/**************************************************************************
 *   Created: 2014/10/16 01:51:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Interaction { namespace OnixsHotspot {

	inline double ConvertToDouble(const OnixS::HotspotItch::Decimal &source) {
		double result = .0;
		source.toNumber(&result);
		return result;
	}

	inline uint32_t ConvertToUInt(const OnixS::HotspotItch::Decimal &source) {
		uint32_t result = 0;
		source.toNumber(&result);
		return result;
	}

} } }