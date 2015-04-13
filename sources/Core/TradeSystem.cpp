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
#include "RiskControl.hpp"
#include "Security.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

TradeSystem::Error::Error(const char *what) throw()
	: Base::Error(what) {
	//...//
}

TradeSystem::OrderParamsError::OrderParamsError(
		const char *what,
		const boost::optional<ScaledPrice> &,
		const Qty &,
		const OrderParams &)
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

namespace {
	
	std::string FormatStringId(const std::string &tag) {
		std::string	result("TradeSystem");
		if (!tag.empty()) {
			result += '.';
			result += tag;
		}
		return std::move(result);
	}

}

class TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

	const size_t m_index;

	Context &m_context;

	const std::string m_tag;
	const std::string m_stringId;

	TradeSystem::Log m_log;
	TradeSystem::TradingLog m_tradingLog;

	explicit Implementation(
			size_t index,
			Context &context,
			const std::string &tag)
		: m_index(index),
		m_context(context),
		m_tag(tag),
		m_stringId(FormatStringId(m_tag)),
		m_log(m_stringId, m_context.GetLog()),
		m_tradingLog(m_tag, m_context.GetTradingLog()) {
		//...//
	}

	void ValidateNewOrder(
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params) {
		
		if (price && *price == 0) {
			throw OrderParamsError(
				"Order price can't be zero",
				price,
				qty,
				params);
		}

		if (qty == 0) {
			throw OrderParamsError(
				"Order size can't be zero",
				price,
				qty,
				params);
		}

		if (params.displaySize && *params.displaySize > qty) {
			throw OrderParamsError(
				"Order display size can't be greater then order size",
				price,
				qty,
				params);
		}

		if (params.goodInSeconds && params.goodTillTime) {
			throw OrderParamsError(
				"Good Next Seconds and Good Till Time can't be used at"
					" the same time",
				price,
				qty,
				params);
		}

	}

	void ValidateNewIocOrder(
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params) {

		if (params.goodInSeconds || params.goodTillTime) {
			throw OrderParamsError(
				"Good Next Seconds and Good Till Time can't be used for"
					" Immediate Or Cancel (IOC) order",
				price,
				qty,
				params);
		}

		ValidateNewOrder(qty, price, params);

	}

	void CheckNewBuyOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		
		ValidateNewOrder(qty, price, params);

		m_context.GetRiskControl().CheckNewBuyOrder(
			*m_self,
			security,
			currency,
			qty,
			price,
			timeMeasurement);

	}

	void CheckNewBuyIocOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {

		ValidateNewIocOrder(qty, price, params);

		m_context.GetRiskControl().CheckNewBuyOrder(
			*m_self,
			security,
			currency,
			qty,
			price,
			timeMeasurement);

	}

	void CheckNewSellOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		
		ValidateNewOrder(qty, price, params);

		m_context.GetRiskControl().CheckNewSellOrder(
			*m_self,
			security,
			currency,
			qty,
			price,
			timeMeasurement);

	}

	void CheckNewSellIocOrder(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const boost::optional<ScaledPrice> &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {

		ValidateNewIocOrder(qty, price, params);

		m_context.GetRiskControl().CheckNewSellOrder(
			*m_self,
			security,
			currency,
			qty,
			price,
			timeMeasurement);

	}

	void ConfirmBuyOrder(
			const OrderStatus &orderStatus,
			const Security &security,
			const Currency &currency,
			const Qty &orderQty,
			const boost::optional<ScaledPrice> &orderPrice,
			const Qty &filled,
			double avgPrice,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl().ConfirmBuyOrder(
			orderStatus,
			*m_self,
			security,
			currency,
			orderQty,
			orderPrice,
			filled,
			avgPrice,
			timeMeasurement);
	}

	void ConfirmSellOrder(
			const OrderStatus &orderStatus,
			const Security &security,
			const Currency &currency,
			const Qty &orderQty,
			const boost::optional<ScaledPrice> &orderPrice,
			const Qty &filled,
			double avgPrice,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl().ConfirmSellOrder(
			orderStatus,
			*m_self,
			security,
			currency,
			orderQty,
			orderPrice,
			filled,
			avgPrice,
			timeMeasurement);
	}

};

TradeSystem::TradeSystem(size_t index, Context &context, const std::string &tag)
	: m_pimpl(new Implementation(index, context, tag)) {
	m_pimpl->m_self = this;
}

TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

size_t TradeSystem::GetIndex() const {
	return m_pimpl->m_index;
}

Context & TradeSystem::GetContext() {
	return m_pimpl->m_context;
}

const Context & TradeSystem::GetContext() const {
	return const_cast<TradeSystem *>(this)->GetContext();
}

TradeSystem::Log & TradeSystem::GetLog() const throw() {
	return m_pimpl->m_log;
}

TradeSystem::TradingLog & TradeSystem::GetTradingLog() const throw() {
	return m_pimpl->m_tradingLog;
}

const char * TradeSystem::GetStringStatus(const OrderStatus &code) {

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

const std::string & TradeSystem::GetTag() const {
	return m_pimpl->m_tag;
}

const std::string & TradeSystem::GetStringId() const throw() {
	return m_pimpl->m_stringId;
}

const TradeSystem::Account & TradeSystem::GetAccount() const {
	throw MethodDoesNotImplementedError(
		"Account Cash Balance not implemented");
}

TradeSystem::Position TradeSystem::GetBrokerPostion(
			const std::string &,
			const Symbol &)
		const {
	throw MethodDoesNotImplementedError("Broker Position Info not implemented");
}

void TradeSystem::ForEachBrokerPostion(
			const std::string &,
			const boost::function<bool (const Position &)> &)
		const {
	throw MethodDoesNotImplementedError("Broker Position Info not implemented");
}

void TradeSystem::Connect(const IniSectionRef &conf) {
	if (IsConnected()) {
		return;
	}
	CreateConnection(conf);
}

OrderId TradeSystem::SellAtMarketPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewSellOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);

	try {
		return SendSellAtMarketPrice(
			security,
			currency,
			qty,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmSellOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::Sell(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewSellOrder(
		security,
		currency,
		qty,
		price,
		params,
		timeMeasurement);
	
	try {
		return SendSell(
			security,
			currency,
			qty,
			price,
			params,
			[
				this,
				&security,
				&currency,
				qty,
				price,
				callback,
				timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmSellOrder(
					orderStatus,
					security,
					currency,
					qty,
					price,
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			price,
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::SellAtMarketPriceWithStopPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewSellOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);

	try {
		return SendSellAtMarketPriceWithStopPrice(
			security,
			currency,
			qty,
			stopPrice,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmSellOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::SellImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewSellIocOrder(
		security,
		currency,
		qty,
		price,
		params,
		timeMeasurement);

	try {
		return SendSellImmediatelyOrCancel(
			security,
			currency,
			qty,
			price,
			params,
			[
				this,
				&security,
				&currency,
				qty,
				price,
				callback,
				timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmSellOrder(
					orderStatus,
					security,
					currency,
					qty,
					price,
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			price,
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::SellAtMarketPriceImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewSellIocOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);
	
	try {
		return SendSellAtMarketPriceImmediatelyOrCancel(
			security,
			currency,
			qty,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmSellOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::BuyAtMarketPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewBuyOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);

	try {
		return SendBuyAtMarketPrice(
			security,
			currency,
			qty,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmBuyOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::Buy(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewBuyOrder(
		security,
		currency,
		qty,
		price,
		params,
		timeMeasurement);
	
	try {
		return SendBuy(
			security,
			currency,
			qty,
			price,
			params,
			[
				this,
				&security,
				&currency,
				qty,
				price,
				callback,
				timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmBuyOrder(
					orderStatus,
					security,
					currency,
					qty,
					price,
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			price,
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::BuyAtMarketPriceWithStopPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewBuyOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);

	try {
		return SendBuyAtMarketPriceWithStopPrice(
			security,
			currency,
			qty,
			stopPrice,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmBuyOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::BuyImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewBuyIocOrder(
		security,
		currency,
		qty,
		price,
		params,
		timeMeasurement);

	try {
		return SendBuyImmediatelyOrCancel(
			security,
			currency,
			qty,
			price,
			params,
			[
				this,
				&security,
				&currency,
				qty,
				price,
				callback,
				timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmBuyOrder(
					orderStatus,
					security,
					currency,
					qty,
					price,
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			price,
			0,
			0,
			timeMeasurement);
		throw;
	}

}

OrderId TradeSystem::BuyAtMarketPriceImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewBuyIocOrder(
		security,
		currency,
		qty,
		boost::optional<ScaledPrice>(),
		params,
		timeMeasurement);
	
	try {
		return SendBuyAtMarketPriceImmediatelyOrCancel(
			security,
			currency,
			qty,
			params,
			[this, &security, &currency, qty, callback, timeMeasurement](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
				m_pimpl->ConfirmBuyOrder(
					orderStatus,
					security,
					currency,
					qty,
					boost::optional<ScaledPrice>(),
					filled,
					avgPrice,
					timeMeasurement);
				callback(orderId, orderStatus, filled, remaining, avgPrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			ORDER_STATUS_ERROR,
			security,
			currency,
			qty,
			boost::optional<ScaledPrice>(),
			0,
			0,
			timeMeasurement);
		throw;
	}

}

void TradeSystem::CancelOrder(const OrderId &order) {
	SendCancelOrder(order);
}

void TradeSystem::CancelAllOrders(Security &security) {
	SendCancelAllOrders(security);
}

void TradeSystem::Test() {
	throw MethodDoesNotImplementedError(
		"Trading system does not support testing");
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(
		std::ostream &oss,
		const TradeSystem &tradeSystem) {
	oss << tradeSystem.GetStringId();
	return oss;
}

////////////////////////////////////////////////////////////////////////////////
