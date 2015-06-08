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

	void ValidateNewOrder(const Qty &qty, const OrderParams &params) {

		if (params.displaySize && *params.displaySize > qty) {
			throw OrderParamsError(
				"Order display size can't be greater then order size",
				qty,
				params);
		}

		if (params.goodInSeconds && params.goodTillTime) {
			throw OrderParamsError(
				"Good Next Seconds and Good Till Time can't be used at"
					" the same time",
				qty,
				params);
		}

	}

	void ValidateNewIocOrder(const Qty &qty, const OrderParams &params) {

		if (params.goodInSeconds || params.goodTillTime) {
			throw OrderParamsError(
				"Good Next Seconds and Good Till Time can't be used for"
					" Immediate Or Cancel (IOC) order",
				qty,
				params);
		}

		ValidateNewOrder(qty, params);

	}

	void CheckNewBuyOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewOrder(qty, params);
		m_context.GetRiskControl().CheckNewBuyOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	void CheckNewBuyIocOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewIocOrder(qty, params);
		m_context.GetRiskControl().CheckNewBuyOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	void CheckNewSellOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewOrder(qty, params);
		m_context.GetRiskControl().CheckNewSellOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	void CheckNewSellIocOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewIocOrder(qty, params);
		m_context.GetRiskControl().CheckNewSellOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	void ConfirmBuyOrder(
			RiskControlScope &riskControlScope,
			const OrderStatus &orderStatus,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl().ConfirmBuyOrder(
			riskControlScope,
			orderStatus,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
			timeMeasurement);
	}

	void ConfirmSellOrder(
			RiskControlScope &riskControlScope,
			const OrderStatus &orderStatus,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &tradeQty,
			const ScaledPrice &tradePrice,
			const Qty &remainingQty,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl().ConfirmSellOrder(
			riskControlScope,
			orderStatus,
			security,
			currency,
			orderPrice,
			tradeQty,
			tradePrice,
			remainingQty,
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
	throw MethodDoesNotImplementedError(
		"Broker Position Info is not implemented");
}

void TradeSystem::ForEachBrokerPostion(
			const std::string &,
			const boost::function<bool (const Position &)> &)
		const {
	throw MethodDoesNotImplementedError(
		"Broker Position Info is not implemented");
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	const auto supposedPrice = security.GetBidPriceScaled();

	m_pimpl->CheckNewSellOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);

	try {
		return SendSellAtMarketPrice(
			security,
			currency,
			qty,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmSellOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewSellOrder(
		riskControlScope,
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
			[&, price, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmSellOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	const auto supposedPrice = security.GetBidPriceScaled();

	m_pimpl->CheckNewSellOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);

	try {
		return SendSellAtMarketPriceWithStopPrice(
			security,
			currency,
			qty,
			stopPrice,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmSellOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewSellIocOrder(
		riskControlScope,
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
			[&, price, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmSellOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	const auto supposedPrice = security.GetBidPriceScaled();
	
	m_pimpl->CheckNewSellIocOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);
	
	try {
		return SendSellAtMarketPriceImmediatelyOrCancel(
			security,
			currency,
			qty,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmSellOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	const auto supposedPrice = security.GetAskPriceScaled();

	m_pimpl->CheckNewBuyOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);

	try {
		return SendBuyAtMarketPrice(
			security,
			currency,
			qty,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmBuyOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	m_pimpl->CheckNewBuyOrder(
		riskControlScope,
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
			[&, price, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmBuyOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	const auto supposedPrice = security.GetAskPriceScaled();

	m_pimpl->CheckNewBuyOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);

	try {
		return SendBuyAtMarketPriceWithStopPrice(
			security,
			currency,
			qty,
			stopPrice,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmBuyOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {

	m_pimpl->CheckNewBuyIocOrder(
		riskControlScope,
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
			[&, price, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmBuyOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			0,
			0,
			qty,
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
		RiskControlScope &riskControlScope,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	const auto supposedPrice = security.GetAskPriceScaled();

	m_pimpl->CheckNewBuyIocOrder(
		riskControlScope,
		security,
		currency,
		qty,
		supposedPrice,
		params,
		timeMeasurement);
	
	try {
		return SendBuyAtMarketPriceImmediatelyOrCancel(
			security,
			currency,
			qty,
			params,
			[&, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const OrderStatus &orderStatus,
					const Qty &tradeQty,
					const Qty &remainingQty,
					const ScaledPrice &tradePrice) {
				m_pimpl->ConfirmBuyOrder(
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					tradeQty,
					tradePrice,
					remainingQty,
					timeMeasurement);
				callback(
					orderId,
					orderStatus,
					tradeQty,
					remainingQty,
					tradePrice);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			0,
			0,
			qty,
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

void TradeSystem::OnSettingsUpdate(const IniSectionRef &) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(
		std::ostream &oss,
		const TradeSystem &tradeSystem) {
	oss << tradeSystem.GetStringId();
	return oss;
}

////////////////////////////////////////////////////////////////////////////////
