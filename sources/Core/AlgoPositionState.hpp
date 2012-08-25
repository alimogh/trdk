/**************************************************************************
 *   Created: 2012/07/31 20:48:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once
#include "Algo.hpp"

class TRADER_CORE_API AlgoPositionState : private boost::noncopyable {

public:

	AlgoPositionState();
	virtual ~AlgoPositionState();

};
