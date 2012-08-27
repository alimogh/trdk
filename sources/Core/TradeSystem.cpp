/**************************************************************************
 *   Created: 2012/07/24 10:07:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"

using namespace Trader;

//////////////////////////////////////////////////////////////////////////

TradeSystem::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

TradeSystem::ConnectError::ConnectError(const char *what) throw()
		: Error(what) {
	//...//
}

TradeSystem::ConnectionDoesntExistError::ConnectionDoesntExistError(const char *what) throw()
		: Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

TradeSystem::TradeSystem() {
	//...//
}

TradeSystem::~TradeSystem() {
	//...//
}

const char * TradeSystem::GetStringStatus(OrderStatus code) {

	switch (code) {
		case TradeSystem::ORDER_STATUS_PENDIGN:
			return "pending";
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			return "submitted";
		case TradeSystem::ORDER_STATUS_CANCELLED:
			return "canceled";
		case TradeSystem::ORDER_STATUS_FILLED:
			return "filled";
		case TradeSystem::ORDER_STATUS_INACTIVE:
			return "inactive";
		case TradeSystem::ORDER_STATUS_ERROR:
			return "send error";
		default:
			AssertFail("Unknown order status code");
			return "unknown";
	}

}

//////////////////////////////////////////////////////////////////////////
