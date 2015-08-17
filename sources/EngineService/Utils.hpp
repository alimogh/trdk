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
#include "Common/Exception.hpp"

namespace trdk { namespace EngineService {

	trdk::EngineService::TradingMode Convert(const trdk::TradingMode &);
	void ConvertToUuid(const boost::uuids::uuid &, Uuid &);
	boost::uuids::uuid ConvertFromUuid(const Uuid &);

} }
