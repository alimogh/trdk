/**************************************************************************
 *   Created: 2012/07/24 10:07:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"

using namespace trdk;

//////////////////////////////////////////////////////////////////////////

TradeSystem::Error::Error(const char *what) throw()
		: Base::Error(what) {
	//...//
}

TradeSystem::OrderParamsError::OrderParamsError(
			const char *what,
			Qty,
			const trdk::OrderParams &)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::SendingError::SendingError() throw()
		: Error("Failed to send data to trade system") {
	//...//
}

TradeSystem::ConnectionDoesntExistError::ConnectionDoesntExistError(
			const char *what)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::UnknownAccountError::UnknownAccountError(
			const char *what)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::PositionError::PositionError(
			const char *what)
		throw()
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

	static_assert(
		numberOfOrderStatuses == 6,
		"Changed trade system order status list.");

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

void TradeSystem::Validate(
			Qty qty,
			const OrderParams &params,
			bool isIoc)
		const {

	if (qty == 0) {
		throw OrderParamsError("Order size can't be zero", qty, params);
	}

	if (params.displaySize && *params.displaySize > qty) {
		throw OrderParamsError(
			"Order display size can't be greater then order size",
			qty,
			params);
	}

	if (isIoc && (params.goodInSeconds || params.goodTillTime)) {
		throw OrderParamsError(
			"Good Next Seconds and Good Till Time can't be used for"
				" Immediate Or Cancel (IOC) order",
			qty,
			params);
	} else if (params.goodInSeconds && params.goodTillTime) {
		throw OrderParamsError(
			"Good Next Seconds and Good Till Time can't be used at"
				" the same time",
			qty,
			params);
	}

}

//////////////////////////////////////////////////////////////////////////
