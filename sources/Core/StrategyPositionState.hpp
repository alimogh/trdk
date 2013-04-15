/**************************************************************************
 *   Created: 2012/07/31 20:48:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once
#include "Strategy.hpp"

class TRADER_CORE_API StrategyPositionState : private boost::noncopyable {

public:

	StrategyPositionState();
	virtual ~StrategyPositionState();

};
