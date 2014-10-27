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
#include "FixTrading.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

FixTrading::FixTrading(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: TradeSystem(context, tag),
		m_account(conf.ReadKey("account", "")),
		m_session(GetContext(), "trading", conf),
		m_nextOrderId(1),
		m_ordersCountReportsCounter(0),
		m_currentToSend(&m_toSend.first),
		m_sendThread([&] {SendThreadMain();}) {
	//...//
}

FixTrading::~FixTrading() {
	try {
		m_session.Disconnect();
		{
			const SendLock lock(m_sendMutex);
			Assert(m_currentToSend);
			m_currentToSend = nullptr;
			m_sendCondition.notify_all();
		}
		m_sendThread.join();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void FixTrading::SendThreadMain() {
	try {
		SendLock lock(m_sendMutex);
		while (m_currentToSend) {
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

void FixTrading::Send(const OrderToSend &order) {
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

void FixTrading::ScheduleSend(OrderToSend &order) {
	{
		const SendLock lock(m_sendMutex);
		m_currentToSend->push_back(order);
		m_sendCondition.notify_one();
	}
	order.timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_ENQUEUE);
}

bool FixTrading::IsConnected() const {
	return m_session.IsConnected();
}

void FixTrading::CreateConnection(const IniSectionRef &conf) {
	Assert(!m_session.IsConnected());
	try {
		m_session.Connect(conf, *this);
	} catch (const FixSession::ConnectError &ex) {
		throw ConnectError(ex.what());
	} catch (const FixSession::Error &ex) {
		throw Error(ex.what());
	}
}

OrderId FixTrading::GetMessageOrderId(const fix::Message &message) const {
	return message.getUInt64(fix::FIX40::Tags::ClOrdID);
}

OrderId FixTrading::TakeOrderId(
			const Security &security,
			const Currency &currency,
			const Qty &qty,
			bool isSell,
			const OrderStatusUpdateSlot &callback,
			const TimeMeasurement::Milestones &timeMeasurement) {
	const Order order = {
		false,
		m_nextOrderId++,
		&security,
		currency,
		qty,
		isSell,
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

FixTrading::Order * FixTrading::FindOrder(
			const OrderId &orderId) {
	foreach (auto &order, m_orders) {
		if (order.id == orderId) {
			return &order;
		}
	}
	return nullptr;
}

void FixTrading::DeleteErrorOrder(const OrderId &orderId) throw() {
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

void FixTrading::FlushRemovedOrders() {
	const OrdersWriteLock lock(m_ordersMutex);
	while (!m_orders.empty() && m_orders.front().isRemoved) {
		m_orders.pop_front();
	}
	while (!m_orders.empty() && m_orders.back().isRemoved) {
		m_orders.pop_back();
	}
}

void FixTrading::NotifyOrderUpdate(
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
		ParseLastShares(updateMessage),
		ParseLeavesQty(updateMessage),
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
				const std::string &account,
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
		if (!account.empty()) {
			message.set(fix::FIX40::Tags::Account, account);
		}
	}
}

fix::Message FixTrading::CreateOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	// Creates order FIX-message and sets common fields.
	fix::Message result("D", m_session.GetFixVersion());
	FillOrderMessage(orderId, security, currency, qty, m_account, result);
	return std::move(result);
}

fix::Message & FixTrading::GetPreallocatedOrderMessage(
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
		m_account,
		*m_preallocated.orderMessage);
	return *m_preallocated.orderMessage;
}

fix::Message & FixTrading::GetPreallocatedMarketOrderMessage(
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

fix::Message FixTrading::CreateLimitOrderMessage(
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

void FixTrading::Send(
			fix::Message &message,
			TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SEND);
	m_session.Get().send(&message);
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SENT);
}

OrderId FixTrading::SellAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		true,
		callback,
		timeMeasurement);
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

OrderId FixTrading::Sell(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			trdk::ScaledPrice price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		true,
		callback,
		timeMeasurement);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(order, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId FixTrading::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"FixTrading::SellAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId FixTrading::SellImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::ScaledPrice &price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		true,
		callback,
		timeMeasurement);
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

OrderId FixTrading::SellAtMarketPriceImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	OrderToSend order = {
		TakeOrderId(
			security,
			currency,
			qty,
			true,
			callback,
			timeMeasurement),
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

OrderId FixTrading::BuyAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		false,
		callback,
		timeMeasurement);
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

OrderId FixTrading::Buy(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			trdk::ScaledPrice price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		false,
		callback,
		timeMeasurement);
	try {
		fix::Message order
			= CreateLimitOrderMessage(orderId, security, currency, qty, price);
		order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		order.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(order, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(orderId);
		throw;
	}
	return orderId;
}

OrderId FixTrading::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"FixTrading::BuyAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId FixTrading::BuyImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::ScaledPrice &price,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &orderId = TakeOrderId(
		security,
		currency,
		qty,
		false,
		callback,
		timeMeasurement);
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

OrderId FixTrading::BuyAtMarketPriceImmediatelyOrCancel(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			const trdk::Qty &qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	OrderToSend order = {
		TakeOrderId(
			security,
			currency,
			qty,
			false,
			callback,
			timeMeasurement),
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

void FixTrading::CancelOrder(OrderId orderId) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	fix::Message message("F", m_session.GetFixVersion());
 	message.set(
 			fix::FIX40::Tags::ClOrdID,
 			boost::lexical_cast<std::string>(m_nextOrderId++));
	message.set(
		fix::FIX40::Tags::OrigClOrdID,
		boost::lexical_cast<std::string>(orderId));
	message.set(
		fix::FIX40::Tags::TransactTime,
		fix::Timestamp::utc(),
		fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
	{
		const OrdersReadLock lock(m_ordersMutex);
		const Order *const order = FindOrder(orderId);
		if (!order) {
			GetLog().Warn(
				"FIX Server (%1%) failed to find order with ID %2% to cancel.",
				boost::make_tuple(GetTag(), boost::cref(orderId)));
			return;
		}
		message.set(
			fix::FIX40::Tags::Symbol,
			order->security->GetSymbol().GetSymbol());
		message.set(
			fix::FIX40::Tags::Side,
			order->isSell
				?	fix::FIX40::Values::Side::Sell
				:	fix::FIX40::Values::Side::Buy);
		message.set(fix::FIX40::Tags::Currency, ConvertToIso(order->currency));
	}
	Send(message, timeMeasurement);
}

void FixTrading::CancelAllOrders(trdk::Security &) {
	throw Error("FixTrading::CancelAllOrders not implemented");
}

void FixTrading::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void FixTrading::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void FixTrading::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void FixTrading::OnOrderNew(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		execReport.get(fix::FIX41::Tags::ExecType)
			== fix::FIX41::Values::ExecType::New);
	NotifyOrderUpdate(
		execReport,
		ORDER_STATUS_SUBMITTED,
		"NEW",
		false,
		replyTime);
}

void FixTrading::OnOrderCanceled(
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

void FixTrading::OnOrderRejected(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime,
			const std::string &reason,
			bool isMaxOperationLimitExceeded) {

	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Rejected
			== execReport.get(fix::FIX41::Tags::ExecType));

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

void FixTrading::OnOrderFill(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	NotifyOrderUpdate(execReport, ORDER_STATUS_FILLED, "FILL", true, replyTime);
}
		
void FixTrading::OnOrderPartialFill(
			const fix::Message &execReport,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Fill
			== execReport.get(fix::FIX41::Tags::ExecType));
	NotifyOrderUpdate(
		execReport,
		ORDER_STATUS_FILLED,
		"PARTIAL FILL",
		false,
		replyTime);
}

Qty FixTrading::ParseLastShares(const fix::Message &message) const {
	return message.getInt32(fix::FIX40::Tags::LastShares);
}

Qty FixTrading::ParseLeavesQty(const fix::Message &message) const {
	return message.getInt32(fix::FIX41::Tags::LeavesQty);
}
