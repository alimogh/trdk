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
	
	std::string FormatStringId(
			const std::string &tag,
			const TradingMode &mode) {
		std::string	result("TradeSystem");
		if (!tag.empty()) {
			result += '.';
			result += tag;
		}
		result += '.';
		result += ConvertToString(mode);
		return result;
	}

}

class TradeSystem::Implementation : private boost::noncopyable {

public:

	TradeSystem *m_self;

	const TradingMode m_mode;

	const size_t m_index;

	Context &m_context;

	const std::string m_tag;
	const std::string m_stringId;

	TradeSystem::Log m_log;
	TradeSystem::TradingLog m_tradingLog;

	explicit Implementation(
			const TradingMode &mode,
			size_t index,
			Context &context,
			const std::string &tag)
		: m_mode(mode),
		m_index(index),
		m_context(context),
		m_tag(tag),
		m_stringId(FormatStringId(m_tag, m_mode)),
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

	RiskControlOperationId CheckNewBuyOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewOrder(qty, params);
		return m_context.GetRiskControl(m_mode).CheckNewBuyOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	RiskControlOperationId CheckNewBuyIocOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewIocOrder(qty, params);
		return m_context.GetRiskControl(m_mode).CheckNewBuyOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	RiskControlOperationId CheckNewSellOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewOrder(qty, params);
		return m_context.GetRiskControl(m_mode).CheckNewSellOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	RiskControlOperationId CheckNewSellIocOrder(
			RiskControlScope &riskControlScope,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params,
			const TimeMeasurement::Milestones &timeMeasurement) {
		ValidateNewIocOrder(qty, params);
		return m_context.GetRiskControl(m_mode).CheckNewSellOrder(
			riskControlScope,
			security,
			currency,
			qty,
			price,
			timeMeasurement);
	}

	void ConfirmBuyOrder(
			const RiskControlOperationId &riskControlOperationId,
			RiskControlScope &riskControlScope,
			const OrderStatus &orderStatus,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeInfo *trade,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl(m_mode).ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			orderStatus,
			security,
			currency,
			orderPrice,
			remainingQty,
			trade,
			timeMeasurement);
	}

	void ConfirmSellOrder(
			const RiskControlOperationId &operationId,
			RiskControlScope &riskControlScope,
			const OrderStatus &orderStatus,
			Security &security,
			const Currency &currency,
			const ScaledPrice &orderPrice,
			const Qty &remainingQty,
			const TradeInfo *trade,
			const TimeMeasurement::Milestones &timeMeasurement) {
		m_context.GetRiskControl(m_mode).ConfirmSellOrder(
			operationId,
			riskControlScope,
			orderStatus,
			security,
			currency,
			orderPrice,
			remainingQty,
			trade,
			timeMeasurement);
	}

};

TradeSystem::TradeSystem(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag)
	: m_pimpl(new Implementation(mode, index, context, tag)) {
	m_pimpl->m_self = this;
}

TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

const TradingMode & TradeSystem::GetMode() const {
	return m_pimpl->m_mode;
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
		numberOfOrderStatuses == 9,
		"Changed trade system order status list.");

	switch (code) {
		case ORDER_STATUS_SENT:
			return "sent";
		case ORDER_STATUS_REQUESTED_CANCEL:
			return "req. cancel";
		case ORDER_STATUS_SUBMITTED:
			return "submitted";
		case ORDER_STATUS_CANCELLED:
			return "canceled";
		case ORDER_STATUS_FILLED:
			return "filled";
		case ORDER_STATUS_FILLED_PARTIALLY:
			return "filled part.";
		case ORDER_STATUS_REJECTED:
			return "rejected";
		case ORDER_STATUS_INACTIVE:
			return "inactive";
		case ORDER_STATUS_ERROR:
			return "error";
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

	const auto riskControlOperationId = m_pimpl->CheckNewSellOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmSellOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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
	
	const auto riskControlOperationId = m_pimpl->CheckNewSellOrder(
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
			[&, riskControlOperationId, currency, price, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmSellOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewSellOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmSellOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewSellIocOrder(
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
			[&, riskControlOperationId, currency, price, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmSellOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			qty,
			nullptr,
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
	
	const auto riskControlOperationId = m_pimpl->CheckNewSellIocOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmSellOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Warn(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmSellOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewBuyOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmBuyOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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
	
	const auto riskControlOperationId =  m_pimpl->CheckNewBuyOrder(
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
			[&, riskControlOperationId, currency, price, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmBuyOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewBuyOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmBuyOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewBuyIocOrder(
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
			[&, riskControlOperationId, currency, price, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmBuyOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					price,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			price,
			qty,
			nullptr,
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

	const auto riskControlOperationId = m_pimpl->CheckNewBuyIocOrder(
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
			[&, riskControlOperationId, currency, supposedPrice, timeMeasurement, callback](
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &orderStatus,
					const Qty &remainingQty,
					const TradeInfo *trade) {
				m_pimpl->ConfirmBuyOrder(
					riskControlOperationId,
					riskControlScope,
					orderStatus,
					security,
					currency,
					supposedPrice,
					remainingQty,
					trade,
					timeMeasurement);
				callback(
					orderId,
					tradeSystemOrderId,
					orderStatus,
					remainingQty,
					trade);
			});
	} catch (...) {
		GetLog().Debug(
			"Error while sending order, rollback trading check state...");
		m_pimpl->ConfirmBuyOrder(
			riskControlOperationId,
			riskControlScope,
			ORDER_STATUS_ERROR,
			security,
			currency,
			supposedPrice,
			qty,
			nullptr,
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
