/**************************************************************************
 *   Created: 2014/08/12 23:52:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CurrenexTrading.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

CurrenexTrading::CurrenexTrading(
					const std::string &tag,
					const IniSectionRef &conf,
					Context::Log &log)
		: TradeSystem(tag),
		m_log(log),
		m_session("trading", conf, m_log),
		m_nextOrderId(1),
		m_ordersCountReportsCounter(0) {
	//...//
}

CurrenexTrading::~CurrenexTrading() {
	//...//
}

void CurrenexTrading::Connect(const IniSectionRef &conf) {
	if (m_session.IsConnected()) {
		return;
	}
	try {
		m_session.Connect(conf, *this);
	} catch (const CurrenexFixSession::ConnectError &ex) {
		throw ConnectError(ex.what());
	} catch (const CurrenexFixSession::Error &ex) {
		throw Error(ex.what());
	}
}

OrderId CurrenexTrading::GetMessageOrderId(const fix::Message &message) const {
	return message.getUInt64(fix::FIX40::Tags::ClOrdID);
}

OrderId CurrenexTrading::TakeOrderId(const OrderStatusUpdateSlot &callback) {
	const Order order = {
		false,
		m_nextOrderId++,
		callback
	};
	{
		const OrdersWriteLock lock(m_ordersMutex);
		m_orders.push_back(order);
		if (m_orders.size() >= 1000) {
			if (!(++m_ordersCountReportsCounter % 1000)) {
				m_log.Warn(
					"FIX orders storage too big (%1%): %2% records.",
					boost::make_tuple(GetTag(), m_orders.size()));
				m_ordersCountReportsCounter = 0;
			}
		}
	}
	return order.id;
}

CurrenexTrading::Order * CurrenexTrading::FindOrder(
			const OrderId &orderId) {
	foreach (auto &order, m_orders) {
		if (order.id == orderId) {
			return &order;
		}
	}
	return nullptr;
}

CurrenexTrading::OrderStatusUpdateSlot CurrenexTrading::FindOrderCallback(
			const OrderId &orderId,
			const char *operation,
			bool isOrderCompleted) {
	const OrdersReadLock lock(m_ordersMutex);
	Order *order = FindOrder(orderId);
	if (!order) {
		m_log.Error(
			"FIX Server (%1%) sent unknown %2% order ID %3%.",
			boost::make_tuple(GetTag(), operation, orderId));
		return OrderStatusUpdateSlot();
	}
	Assert(!order->isRemoved);
	order->isRemoved = isOrderCompleted;
	return order->callback;
}

void CurrenexTrading::DeleteErrorOrder(const OrderId &orderId) throw() {
	try {
		{
			const OrdersReadLock lock(m_ordersMutex);
			Order *const order = FindOrder(orderId);
			Assert(order);
			if (!order) {
				return;
			}
			Assert(!order->isRemoved);
			order->isRemoved = true;
		}
		FlushRemovedOrders();
	} catch (...) {
		AssertFailNoException();
	}
}

void CurrenexTrading::FlushRemovedOrders() {
	const OrdersWriteLock lock(m_ordersMutex);
	while (!m_orders.empty() && m_orders.front().isRemoved) {
		m_orders.pop_front();
	}
	while (!m_orders.empty() && m_orders.back().isRemoved) {
		m_orders.pop_back();
	}
}

void CurrenexTrading::NotifyOrderUpdate(
			const fix::Message &updateMessage,
			const OrderStatus &status,
			const char *operation,
			bool isOrderCompleted) {
	const auto &orderId = GetMessageOrderId(updateMessage);
	const auto &callback
		= FindOrderCallback(orderId, operation, isOrderCompleted);
	if (!callback) {
		return;
	}
	callback(
		orderId,
		status,
		updateMessage.getInt32(fix::FIX40::Tags::LastShares),
		updateMessage.getInt32(fix::FIX41::Tags::LeavesQty),
		updateMessage.getDouble(fix::FIX40::Tags::AvgPx));
	if (isOrderCompleted) {
		FlushRemovedOrders();
	}
}

fix::Message CurrenexTrading::CreateOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	// Creates order FIX-message and sets common fields.
	fix::Message result("D", m_session.GetFixVersion());
	result.set(
		fix::FIX40::Tags::ClOrdID,
		boost::lexical_cast<std::string>(orderId));
	result.set(
		fix::FIX40::Tags::HandlInst,
		fix::FIX40::Values::HandlInst::Automated_execution_order_private_no_Broker_intervention);
	result.set(fix::FIX40::Tags::Currency, ConvertToIso(currency));
	result.set(fix::FIX40::Tags::Symbol, security.GetSymbol().GetSymbol());
	result.set(
		fix::FIX40::Tags::TransactTime,
		fix::Timestamp::utc(),
		fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
	result.set(fix::FIX40::Tags::OrderQty, qty);
	return std::move(result);
}

fix::Message CurrenexTrading::CreateMarketOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	fix::Message order = CreateOrderMessage(orderId, security, currency, qty);
	order.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Market);
	return std::move(order);
}

fix::Message CurrenexTrading::CreateLimitOrderMessage(
			const OrderId &orderId,
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &price) {
	fix::Message order = CreateOrderMessage(orderId, security, currency, qty);
	order.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Limit);
	order.set(
		fix::FIX40::Tags::Price,
		security.DescalePrice(price),
		security.GetPricePrecision());
	return std::move(order);
}

OrderId CurrenexTrading::SellAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId CurrenexTrading::Sell(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Sell not implemented");
}

OrderId CurrenexTrading::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexTrading::SellAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexTrading::SellImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::ScaledPrice &price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId CurrenexTrading::SellAtMarketPriceImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId CurrenexTrading::BuyAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId CurrenexTrading::Buy(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Buy not implemented");
}

OrderId CurrenexTrading::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexTrading::BuyAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexTrading::BuyImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::ScaledPrice &price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId CurrenexTrading::BuyAtMarketPriceImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	const auto &orderId = TakeOrderId(callback);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		m_session.Get().send(&order);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

void CurrenexTrading::CancelOrder(OrderId) {
	throw Error("CurrenexTrading::CancelOrder not implemented");
}

void CurrenexTrading::CancelAllOrders(trdk::Security &) {
	throw Error("CurrenexTrading::CancelAllOrders not implemented");
}

void CurrenexTrading::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void CurrenexTrading::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void CurrenexTrading::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void CurrenexTrading::onInboundApplicationMsg(
			fix::Message &message,
			fix::Session *session) {
	
	Assert(session == &m_session.Get());
	UseUnused(session);

	if (message.type() != "8") {
		return;
	}

	const auto &execTransType = message.get(fix::FIX40::Tags::ExecTransType);

	if (execTransType == fix::FIX40::Values::ExecTransType::New) {
		
		const auto &execType = message.get(fix::FIX41::Tags::ExecType);
		const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
		if (execType == fix::FIX41::Values::ExecType::New) {
			
			if (ordStatus == fix::FIX40::Values::OrdStatus::New) {
				OnOrderNew(message);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

			if (ordStatus == fix::FIX40::Values::OrdStatus::Canceled) {
				OnOrderCanceled(message);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Fill) {

			if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
				OnOrderPartialFill(message);
				return;
			} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
				OnOrderFill(message);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Suspended) {
			
			//...//
		
		} else if (execType == fix::FIX41::Values::ExecType::Rejected) {
			OnOrderRejected(message);
			return;
		}
		
	} else if (execTransType == fix::FIX40::Values::ExecTransType::Status) {
		
		//...//

	}

	m_log.Error(
		"FIX Server sent unknown Execution Report: \"%1%\".",
		boost::make_tuple(boost::cref(message)));

}

void CurrenexTrading::OnOrderNew(const fix::Message &execReport) {
	AssertEq("8", execReport.type());
	Assert(
		execReport.get(fix::FIX41::Tags::ExecType)
			== fix::FIX41::Values::ExecType::New);
	Assert(
		fix::FIX41::Values::OrdStatus::New
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(execReport, ORDER_STATUS_SUBMITTED, "NEW", false);
}

void CurrenexTrading::OnOrderCanceled(const fix::Message &execReport) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Cancelled
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Canceled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(execReport, ORDER_STATUS_CANCELLED, "CANCELED", true);
}	

void CurrenexTrading::OnOrderRejected(const fix::Message &execReport) {

	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Rejected
			== execReport.get(fix::FIX41::Tags::ExecType));

	const std::string &reason = execReport.get(fix::FIX40::Tags::Text);
	m_log.Error(
		"FIX Server (%1%) Rejected order %2%: \"%3%\".",
		boost::make_tuple(
			GetTag(),
			GetMessageOrderId(execReport),
			boost::cref(reason)));

	NotifyOrderUpdate(execReport, ORDER_STATUS_ERROR, "REJECTED", true);

}

void CurrenexTrading::OnOrderFill(const fix::Message &execReport) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Fill
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Filled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(execReport, ORDER_STATUS_FILLED, "FILL", true);
}
		
void CurrenexTrading::OnOrderPartialFill(const fix::Message &execReport) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Fill
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Partially_filled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(execReport, ORDER_STATUS_FILLED, "PARTIAL FILL", false);
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult CreateCurrenexTrading(
			const std::string &tag,
			const IniSectionRef &configuration,
			Context::Log &log) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new CurrenexTrading(tag, configuration, log));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
