/**************************************************************************
 *   Created: 2015/07/21 03:20:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/


#pragma once

#include "EngineService.h"
#include "Core/Types.hpp"

namespace trdk { namespace EngineService {

	template<typename Result>
	void Convert(const trdk::TradingMode &source, Result &result) {
		static_assert(numberOfTradingModes == 2, "List changed.");
		switch (source) {
			default:
				AssertEq(trdk::TRADING_MODE_LIVE, source);
				break;
			case trdk::TRADING_MODE_LIVE:
				result.set_mode(EngineService::TRADING_MODE_LIVE);
				break;
			case trdk::TRADING_MODE_PAPER:
				result.set_mode(EngineService::TRADING_MODE_PAPER);
				break;
		}
	}

	void ConvertToUuid(const boost::uuids::uuid &, Uuid &);
	boost::uuids::uuid ConvertFromUuid(const Uuid &);

} }
