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
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

FixTrading::FixTrading(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: TradeSystem(mode, index, context, tag),
	m_account(conf.ReadKey("account", "")),
	m_session(GetContext(), GetLog(), conf),
	m_nextOrderId(1),
	m_ordersCountReportsCounter(0),
	m_currentToSend(&m_toSend.first),
	m_sendThread([&] {SendThreadMain();}) {
	//...//
}

FixTrading::~FixTrading() {
	try {
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
					EventsLog::BroadcastUnhandledException(
						__FUNCTION__,
						__FILE__,
						__LINE__);
					throw;
				}
				toSend.clear();
				lock.lock();
			}
			m_sendCondition.wait(lock);
		}
	} catch (...) {
		EventsLog::BroadcastUnhandledException(
			__FUNCTION__,
			__FILE__,
			__LINE__);
		throw;
	}
}

void FixTrading::Send(const OrderToSend &order) {
	try {
		TimeMeasurement::Milestones timeMeasurement = order.timeMeasurement;
		timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_PACK);
		fix::Message &message = GetPreallocatedMarketOrderMessage(
			order.clOrderId,
			*order.security,
			order.currency,
			order.qty,
			order.params);
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

OrderId FixTrading::ParseOrderId(const fix::StringRef &clOrderId) {
	for (size_t i = clOrderId.size(); i > 0; --i) {
		if (clOrderId[i - 1] == '.') {
			return boost::lexical_cast<OrderId>(
				&clOrderId[i],
				clOrderId.size() - i);
		}
	}
	return 0;
}

OrderId FixTrading::GetMessageClOrderId(const fix::Message &message) {
	return ParseOrderId(message.getStringRef(fix::FIX40::Tags::ClOrdID));
}

OrderId FixTrading::GetMessageOrigClOrderId(const fix::Message &message) const {
	return ParseOrderId(message.getStringRef(fix::FIX40::Tags::OrigClOrdID));
}

namespace {
	std::string GenerateClOrderId(const OrderId &orderId) {
		boost::format result(
#			ifdef DEV_VER
				"DEV_VER.%1$02d%2$02d%3$02d.%4%"
#			else
				"%1$02d%2$02d%3$02d.%4%"
#			endif
		);
		const auto &now
			= boost::posix_time::second_clock::local_time().time_of_day();
		result % now.hours() % now.minutes() % now.seconds() % orderId;
		return std::move(result.str());
	}
}

FixTrading::Order FixTrading::TakeOrderId(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		bool isSell,
		const OrderType &type,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback,
		const TimeMeasurement::Milestones &timeMeasurement) {
	const auto orderId = m_nextOrderId++;
	const Order order = {
		false,
		orderId,
		std::string(),
		GenerateClOrderId(orderId),
		&security,
		currency,
		qty,
		isSell,
		type,
		params,
		callback,
		timeMeasurement
	};
	{
		const OrdersWriteLock lock(m_ordersMutex);
		m_orders.push_back(order);
		if (m_orders.size() >= 1000) {
			if (!(++m_ordersCountReportsCounter % 1000)) {
				GetLog().Warn(
					"Orders storage too big: %1% records.",
					m_orders.size());
				m_ordersCountReportsCounter = 0;
			}
		}
	}
	return order;
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

const FixTrading::Order * FixTrading::FindOrder(
			const OrderId &orderId)
		const {
	return const_cast<FixTrading *>(this)->FindOrder(orderId);
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
		const OrderId &orderId,
		const OrderStatus &status,
		const char *operation,
		bool isOrderCompleted,
		const TimeMeasurement::Milestones::TimePoint &replyTime) {
	
	Order orderCopy;
	TradeInfo tradeData = {};
	TradeInfo *tradeInfo = nullptr;
	{
		const OrdersReadLock lock(m_ordersMutex);
		Order *const order = FindOrder(orderId);
		if (!order) {
			GetLog().Error(
				"Unknown %1% order ID %2% received.",
				operation,
				orderId);
			return;
		}
#		if defined(BOOST_WINDOWS)
			//! @sa TRDK-124 why Order ID disabled from windows
			if (order->tradeSystemId.empty()) {
				order->tradeSystemId = "<see TRDK-124>";
			}
#		else
			if (order->tradeSystemId.empty()) {
				order->tradeSystemId
					= (std::string)updateMessage.get(fix::FIX40::Tags::OrderID);
 			} else {
 				AssertEq(
 					updateMessage.get(fix::FIX40::Tags::OrderID),
 					order->tradeSystemId);
 			}
#		endif
		Assert(!order->isRemoved);
		order->isRemoved = isOrderCompleted;
		if (status == ORDER_STATUS_FILLED) {
			AssertGe(
				order->qty,
				order->filledQty + ParseLeavesQty(updateMessage));
			tradeInfo = &tradeData;
			tradeInfo->qty = ParseLastShares(updateMessage);
			order->filledQty += tradeData.qty;
		}
		orderCopy = *order;
	}

	if (tradeInfo) {
#		if defined(BOOST_WINDOWS)
			//! @sa TRDK-146 why Order ID disabled from windows
			tradeInfo->id = "<see TRDK-146>";
#		else
			tradeInfo->id
				= (std::string)updateMessage.get(fix::FIX42::Tags::ExecID);
#		endif
		tradeInfo->price = orderCopy.security->ScalePrice(
			updateMessage.getDouble(fix::FIX40::Tags::LastPx));
	}

	orderCopy.timeMeasurement.Add(
		TimeMeasurement::TSM_ORDER_REPLY_RECEIVED,
		replyTime);
	OnOrderStateChanged(
		orderCopy.tradeSystemId,
		status,
		orderCopy,
		tradeInfo);
	if (isOrderCompleted) {
		FlushRemovedOrders();
	}
	orderCopy.timeMeasurement.Measure(
		TimeMeasurement::TSM_ORDER_REPLY_PROCESSED);

}

void FixTrading::OnOrderStateChanged(
		const std::string &borkerOrderId,
		const OrderStatus &status,
		const Order &order,
		const TradeInfo *trade) {
	AssertGe(order.qty, order.filledQty);
	order.callback(
		order.id,
		borkerOrderId,
		status,
		order.qty - order.filledQty,
		trade);
}

void FixTrading::FillOrderMessage(
		const std::string &clOrderId,
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const std::string &account,
		const OrderParams &params,
		fix::Message &message)
		const {

	Assert(!clOrderId.empty());
	message.set(fix::FIX40::Tags::ClOrdID, clOrderId);

	if (params.orderIdToReplace) {
		AssertEq("G", message.type());
		OrdersReadLock lock(m_ordersMutex);
		const Order *const order = FindOrder(*params.orderIdToReplace);
		if (!order) {
			lock.unlock();
			GetLog().Warn(
				"FIX Server failed to find order with ID %1%"
					" to cancel/replace.",
				*params.orderIdToReplace);
			throw Error("Failed to find order to cancel/replace");
		}
		AssertEq(
			*params.orderIdToReplace,
			ParseOrderId(
				fix::StringRef(
					order->clOrderId.c_str(),
					order->clOrderId.size())));
		message.set(fix::FIX40::Tags::OrigClOrdID, order->clOrderId);
	} else {
		AssertEq("D", message.type());
	}

	message.set(
		fix::FIX40::Tags::HandlInst,
		fix::FIX40::Values::HandlInst::Automated_execution_order_private_no_Broker_intervention);
	message.set(fix::FIX40::Tags::Currency, ConvertToIso(currency));
	message.set(fix::FIX40::Tags::Symbol, security.GetSymbol().GetSymbol());
	message.set(
		fix::FIX40::Tags::TransactTime,
		fix::Timestamp::utc(),
		fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
	message.set(fix::FIX40::Tags::OrderQty, qty, 2);
	if (params.minTradeQty) {
		message.set(fix::FIX40::Tags::MinQty, *params.minTradeQty, 2);
	}

	if (!account.empty()) {
		message.set(fix::FIX40::Tags::Account, account);
	}

}

fix::Message FixTrading::CreateOrderMessage(
		const std::string &clOrderId,
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params) {
	// Creates order FIX-message and sets common fields.
	fix::Message result(
		!params.orderIdToReplace ? "D" : "G",
		m_session.GetFixVersion());
	FillOrderMessage(
		clOrderId,
		security,
		currency,
		qty,
		m_account,
		params,
		result);
	return result;
}

fix::Message & FixTrading::GetPreallocatedOrderMessage(
		const std::string &clOrderId,
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params) {
	if (!m_preallocated.orderMessage) {
		m_preallocated.orderMessage.reset(
			new fix::Message("D", m_session.GetFixVersion()));
	} else {
		m_preallocated.orderMessage->clear();
	}
	Assert(!params.orderIdToReplace);
	FillOrderMessage(
		clOrderId,
		security,
		currency,
		qty,
		m_account,
		params,
		*m_preallocated.orderMessage);
	return *m_preallocated.orderMessage;
}

fix::Message & FixTrading::GetPreallocatedMarketOrderMessage(
		const std::string &clOrderId,
		const Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params) {
	fix::Message &result = GetPreallocatedOrderMessage(
		clOrderId,
		security,
		currency,
		qty,
		params);
	result.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Market);
	return result;
}

void FixTrading::Send(
			fix::Message &message,
			const TimeMeasurement::Milestones &timeMeasurement) {
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SEND);
	m_session.Get().send(&message);
	timeMeasurement.Measure(TimeMeasurement::TSM_ORDER_SENT);
}

OrderId FixTrading::SendSellAtMarketPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		true,
		ORDER_TYPE_DAY_MARKET,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateMarketOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendSell(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		true,
		ORDER_TYPE_DAY_LIMIT,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateLimitOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			price,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendSellAtMarketPriceWithStopPrice(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &/*stopPrice*/,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw Error(
		"FixTrading::SendSellAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId FixTrading::SendSellImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		true,
		ORDER_TYPE_IOC_LIMIT,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateLimitOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			price,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Sell);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendSellAtMarketPriceImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const  auto &order = TakeOrderId(
		security,
		currency,
		qty,
		true,
		ORDER_TYPE_IOC_MARKET,
		params,
		callback,
		timeMeasurement);
	OrderToSend orderToSend = {
		order.id,
		order.clOrderId,
		&security,
		currency,
		qty,
		params,
		true,
		timeMeasurement
	};
	try {
		ScheduleSend(orderToSend);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendBuyAtMarketPrice(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		false,
		ORDER_TYPE_DAY_MARKET,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateMarketOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendBuy(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		false,
		ORDER_TYPE_DAY_LIMIT,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateLimitOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			price,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Day);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendBuyAtMarketPriceWithStopPrice(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &/*stopPrice*/,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw Error(
		"FixTrading::SendBuyAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId FixTrading::SendBuyImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		false,
		ORDER_TYPE_IOC_LIMIT,
		params,
		callback,
		timeMeasurement);
	try {
		fix::Message message = CreateLimitOrderMessage(
			order.clOrderId,
			security,
			currency,
			qty,
			price,
			params);
		message.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
		message.set(
			fix::FIX40::Tags::TimeInForce,
			fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
		Send(message, timeMeasurement);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

OrderId FixTrading::SendBuyAtMarketPriceImmediatelyOrCancel(
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &callback) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	const auto &order = TakeOrderId(
		security,
		currency,
		qty,
		false,
		ORDER_TYPE_IOC_MARKET,
		params,
		callback,
		timeMeasurement);
	OrderToSend orderToSend = {
		order.id,
		order.clOrderId,
		&security,
		currency,
		qty,
		params,
		false,
		timeMeasurement
	};
	try {
		ScheduleSend(orderToSend);
	} catch (...) {
		DeleteErrorOrder(order.id);
		throw;
	}
	return order.id;
}

void FixTrading::SendCancelOrder(const OrderId &orderId) {
	TimeMeasurement::Milestones timeMeasurement
		= GetContext().StartTradeSystemTimeMeasurement();
	fix::Message message("F", m_session.GetFixVersion());
	message.set(fix::FIX40::Tags::ClOrdID, GenerateClOrderId(m_nextOrderId++));
	message.set(
		fix::FIX40::Tags::TransactTime,
		fix::Timestamp::utc(),
		fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
	{
		OrdersReadLock lock(m_ordersMutex);
		const Order *const order = FindOrder(orderId);
		if (!order) {
			lock.unlock();
			GetLog().Warn(
				"Failed to find order with ID %1% to cancel.",
				orderId);
			return;
		}
		message.set(fix::FIX40::Tags::OrigClOrdID, order->clOrderId);
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

void FixTrading::SendCancelAllOrders(Security &) {
	throw Error("FixTrading::SendCancelAllOrders not implemented");
}

void FixTrading::Test() {
	fix::Message message("1", m_session.GetFixVersion());
	message.set(
		fix::FIX40::Tags::TestReqID,
		TRDK_BUILD_IDENTITY);
	m_session.Get().send(&message);
	GetLog().Info("Test request sent: \"%1%\".", message);
}

void FixTrading::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {

	Assert(session == &m_session.Get());

	m_session.LogStateChange(newState, prevState, *session);
	if (
			prevState == fix::SessionState::LogoutInProgress
			&& newState == fix::SessionState::Disconnected) {
		OnLogout();
	}
	
	if (newState == fix::SessionState::Disconnected) {
		
		m_session.Reconnect();
	
	} else if (
			prevState != fix::SessionState::Active
			&& newState == fix::SessionState::Active) {

		for ( ; ; ) {
		
			Order orderCopy;
			bool isFound = false;
			{
				const OrdersReadLock lock(m_ordersMutex);
				foreach (auto &order, m_orders) {
					if (order.isRemoved) {
						continue;
					}
					order.isRemoved = true;
					orderCopy = order;
					isFound = true;
					break;
				}
			}
			if (!isFound) {
				break;
			}

			GetTradingLog().Write(
				"canceling order %1% at reconnect",
				[&](TradingRecord &record) {
					record % orderCopy.id;
				});

			orderCopy.callback(
				orderCopy.id,
				orderCopy.tradeSystemId,
				ORDER_STATUS_REJECTED,
				orderCopy.qty - orderCopy.filledQty,
				nullptr);

		}

		FlushRemovedOrders();

	}

}

void FixTrading::onError(
		fix::ErrorReason::Enum reason,
		const std::string &description,
		fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
	if (reason == fix::ErrorReason::MsgSeqNumTooLow) {
		m_session.ResetLocalSequenceNumbers(true, false);
	}
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
	NotifyOrderUpdate(
		execReport,
		GetMessageClOrderId(execReport),
		ORDER_STATUS_SUBMITTED,
		"NEW",
		false,
		replyTime);
}

void FixTrading::OnOrderCanceled(
			const fix::Message &execReport,
			const OrderId &orderId,
			const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	Assert(
		fix::FIX41::Values::ExecType::Cancelled
				== execReport.get(fix::FIX41::Tags::ExecType)
			|| fix::FIX41::Values::OrdStatus::Expired
				== execReport.get(fix::FIX40::Tags::OrdStatus));
	Assert(
		fix::FIX41::Values::OrdStatus::Canceled
				== execReport.get(fix::FIX40::Tags::OrdStatus)
			|| fix::FIX41::Values::OrdStatus::Expired
				== execReport.get(fix::FIX40::Tags::OrdStatus));
	NotifyOrderUpdate(
		execReport,
		orderId,
		ORDER_STATUS_CANCELLED,
		"CANCELED",
		true,
		replyTime);
}	

void FixTrading::OnOrderRejected(
		const fix::Message &execReport,
		const TimeMeasurement::Milestones::TimePoint &replyTime,
		const OrderStatus &status,
		const std::string &reason) {

	AssertEq("8", execReport.type());

	GetLog().Error(
		"Order %1% rejected by server: \"%2%\". Status: %3%.",
		GetMessageClOrderId(execReport),
		reason,
		status);

	NotifyOrderUpdate(
		execReport,
		GetMessageClOrderId(execReport),
		status,
		"REJECTED",
		true,
		replyTime);

}

void FixTrading::OnOrderFill(
		const fix::Message &execReport,
		const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	NotifyOrderUpdate(
		execReport,
		GetMessageClOrderId(execReport),
		ORDER_STATUS_FILLED,
		"FILL",
		true,
		replyTime);
}
		
void FixTrading::OnOrderPartialFill(
		const fix::Message &execReport,
		const TimeMeasurement::Milestones::TimePoint &replyTime) {
	AssertEq("8", execReport.type());
	NotifyOrderUpdate(
		execReport,
		GetMessageClOrderId(execReport),
		ORDER_STATUS_FILLED,
		"PARTIAL FILL",
		false,
		replyTime);
}

Qty FixTrading::ParseLastShares(const fix::Message &message) const {
	return message.getDouble(fix::FIX40::Tags::LastShares);
}

Qty FixTrading::ParseLeavesQty(const fix::Message &message) const {
	return message.getDouble(fix::FIX41::Tags::LeavesQty);
}
