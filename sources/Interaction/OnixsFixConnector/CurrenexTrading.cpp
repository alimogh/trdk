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
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: TradeSystem(context, tag),
		m_session(GetContext(), "trading", conf),
		m_nextOrderId(1),
		m_ordersCountReportsCounter(0),
		m_currentToSend(&m_toSend.first),
		m_sendThread([&] {SendThreadMain();}) {
	//...//
}

CurrenexTrading::~CurrenexTrading() {
	//...//
}

void CurrenexTrading::SendThreadMain() {
	try {
		SendLock lock(m_sendMutex);
		for ( ; ; ) {
			while (!m_currentToSend->empty()) {
				auto &toSend = *m_currentToSend;
				m_currentToSend = &m_toSend.first == m_currentToSend
					?	&m_toSend.second
					:	&m_toSend.first;
				lock.unlock();
				try {
					foreach (const auto &order, toSend) {
						Send(order);
					}
				} catch (...) {
					Log::RegisterUnhandledException(
						__FUNCTION__,
						__FILE__,
						__LINE__,
						false);
					throw;
				}
				toSend.clear();
				lock.lock();
			}
			m_sendCondition.wait(lock);
		}
	} catch (...) {
		Log::RegisterUnhandledException(
			__FUNCTION__,
			__FILE__,
			__LINE__,
			false);
		throw;
	}
}

void CurrenexTrading::Send(const OrderToSend &order) {
	try {
		TimeMeasurement::Milestones timeMeasurement = order.timeMeasurement;
		timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_PACK);
		fix::Message &message = GetPreallocatedMarketOrderMessage(
			order.id,
			*order.security,
			order.currency,
			order.qty);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		message.set(
			fix::FIX40::Tags::Side,
			order.isSell
				?	fix::FIX40::Values::Side::Sell
				:	fix::FIX40::Values::Side::Buy);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
}

void CurrenexTrading::ScheduleSend(OrderToSend &order) {
	{
		const SendLock lock(m_sendMutex);
		m_currentToSend->push_back(order);
		m_sendCondition.notify_one();
	}
	order.timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_ENQUEUE);
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

OrderId CurrenexTrading::TakeOrderId(
			const OrderStatusUpdateSlot &callback,
			const TimeMeasurement::Milestones &timeMeasurement) {
	const Order order = {
		false,
		m_nextOrderId++,
		callback,
		timeMeasurement
	};
	{
		const OrdersWriteLock lock(m_ordersMutex);
		m_orders.push_back(order);
		if (m_orders.size() >= 1000) {
			if (!(++m_ordersCountReportsCounter % 1000)) {
				GetLog().Warn(
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
			bool isOrderCompleted,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	
	const auto &orderId = GetMessageOrderId(updateMessage);
	
	OrderStatusUpdateSlot callback;
	TimeMeasurement::Milestones timeMeasurement;
	{
		const OrdersReadLock lock(m_ordersMutex);
		Order *const order = FindOrder(orderId);
		if (!order) {
			GetLog().Error(
				"FIX Server (%1%) sent unknown %2% order ID %3%.",
				boost::make_tuple(GetTag(), operation, orderId));
			return;
		}
		Assert(!order->isRemoved);
		timeMeasurement = order->timeMeasurement;
		order->isRemoved = isOrderCompleted;
		callback = order->callback;
	}
	Assert(callback);
	Assert(timeMeasurement);

	timeMeasurement.Add(TimeMeasurement::TSM_ORDER_REPLY_RECEIVED, replyTime);
	callback(
		orderId,
		status,
		updateMessage.getInt32(fix::FIX40::Tags::LastShares),
		updateMessage.getInt32(fix::FIX41::Tags::LeavesQty),
		updateMessage.getDouble(fix::FIX40::Tags::AvgPx));
	if (isOrderCompleted) {
		FlushRemovedOrders();
	}
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_REPLY_PROCESSED);

}

namespace {
	void FillOrderMessage(
				const OrderId &orderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty,
				fix::Message &message) {
		message.set(
			fix::FIX40::Tags::ClOrdID,
			boost::lexical_cast<std::string>(orderId));
		message.set(
			fix::FIX40::Tags::HandlInst,
			fix::FIX40::Values::HandlInst::Automated_execution_order_private_no_Broker_intervention);
		message.set(fix::FIX40::Tags::Currency, ConvertToIso(currency));
		message.set(fix::FIX40::Tags::Symbol, security.GetSymbol().GetSymbol());
		message.set(
			fix::FIX40::Tags::TransactTime,
			fix::Timestamp::utc(),
			fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
		message.set(fix::FIX40::Tags::OrderQty, qty);
	}
}

fix::Message CurrenexTrading::CreateOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	// Creates order FIX-message and sets common fields.
	fix::Message result("D", m_session.GetFixVersion());
	FillOrderMessage(orderId, security, currency, qty, result);
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

fix::Message & CurrenexTrading::GetPreallocatedOrderMessage(
				const OrderId &orderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty) {
	if (!m_preallocated.orderMessage) {
		m_preallocated.orderMessage.reset(
			new fix::Message("D", m_session.GetFixVersion()));
	} else {
		m_preallocated.orderMessage->clear();
	}
	FillOrderMessage(
		orderId,
		security,
		currency,
		qty,
		*m_preallocated.orderMessage);
	return *m_preallocated.orderMessage;
}

fix::Message & CurrenexTrading::GetPreallocatedMarketOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	fix::Message &result
		= GetPreallocatedOrderMessage(orderId, security, currency, qty);
	result.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Market);
	return result;
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

void CurrenexTrading::Send(
			fix::Message &message,
			TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SEND);
	m_session.Get().send(&message);
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SENT);
}

OrderId CurrenexTrading::SellAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(callback, timeMeasurement);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		Send(order, timeMeasurement);
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
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(callback, timeMeasurement);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		Send(order, timeMeasurement);
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
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	OrderToSend order = {
		TakeOrderId(callback, timeMeasurement),
		&security,
		currency,
		qty,
		true,
		timeMeasurement
	};
	try {
		ScheduleSend(order);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId CurrenexTrading::BuyAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(callback, timeMeasurement);
	try {
		fix::Message order
			= CreateMarketOrderMessage(orderId, security, currency, qty);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		Send(order, timeMeasurement);
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
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(callback, timeMeasurement);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		Send(order, timeMeasurement);
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
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	OrderToSend order = {
		TakeOrderId(callback, timeMeasurement),
		&security,
		currency,
		qty,
		false,
		timeMeasurement
	};
	try {
		ScheduleSend(order);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

void CurrenexTrading::CancelOrder(OrderId) {
	//...//
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

	const auto &replyTime = TimeMeasurement::Milestones::GetNow();
	
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
				OnOrderNew(message, replyTime);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

			if (ordStatus == fix::FIX40::Values::OrdStatus::Canceled) {
				OnOrderCanceled(message, replyTime);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Fill) {

			if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
				OnOrderPartialFill(message, replyTime);
				return;
			} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
				OnOrderFill(message, replyTime);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Suspended) {
			
			//...//
		
		} else if (execType == fix::FIX41::Values::ExecType::Rejected) {
			OnOrderRejected(message, replyTime);
			return;
		}
		
	} else if (execTransType == fix::FIX40::Values::ExecTransType::Status) {
		
		//...//

	}

	GetLog().Error(
		"FIX Server sent unknown Execution Report: \"%1%\".",
		boost::make_tuple(boost::cref(message)));

}

void CurrenexTrading::OnOrderNew(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		execReport.get(fix::FIX41::Tags::ExecType)
			== fix::FIX41::Values::ExecType::New);
	Assert(
		fix::FIX41::Values::OrdStatus::New
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(
		execReport,
		ORDER_STATUS_SUBMITTED,
		"NEW",
		false,
		replyTime);
}

void CurrenexTrading::OnOrderCanceled(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Cancelled
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Canceled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(
		execReport,
		ORDER_STATUS_CANCELLED,
		"CANCELED",
		true,
		replyTime);
}	

void CurrenexTrading::OnOrderRejected(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {

	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Rejected
			== execReport.get(fix::FIX41::Tags::ExecType));

	const std::string &reason = execReport.get(fix::FIX40::Tags::Text);
	const bool isMaxOperationLimitExceeded = boost::iequals(
		reason,
		"maximum operation limit exceeded");
	GetLog().Error(
		"FIX Server (%1%) Rejected order %2%: \"%3%\".",
		boost::make_tuple(
			GetTag(),
			GetMessageOrderId(execReport),
			boost::cref(reason)));

	NotifyOrderUpdate(
		execReport,
		isMaxOperationLimitExceeded
			?	ORDER_STATUS_INACTIVE
			:	ORDER_STATUS_ERROR,
		"REJECTED",
		true,
		replyTime);

}

void CurrenexTrading::OnOrderFill(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Fill
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Filled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(execReport, ORDER_STATUS_FILLED, "FILL", true, replyTime);
}
		
void CurrenexTrading::OnOrderPartialFill(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Fill
			== execReport.get(fix::FIX41::Tags::ExecType));
	Assert(
		fix::FIX41::Values::OrdStatus::Partially_filled
			== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(
		execReport,
		ORDER_STATUS_FILLED,
		"PARTIAL FILL",
		false,
		replyTime);
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateCurrenexTrading(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new CurrenexTrading(context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
