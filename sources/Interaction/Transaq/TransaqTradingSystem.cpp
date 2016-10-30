/**************************************************************************
 *   Created: 2016/10/30 18:17:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqTradingSystem.hpp"
#include "Core/Security.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

namespace pt = boost::posix_time;

Transaq::TradingSystem::TradingSystem(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: trdk::TradingSystem(mode, index, context, tag)
	, TradingConnector(GetContext(), GetLog(), conf)
	, m_connectorContext(GetContext(), GetLog()) {
	//...//
}

Transaq::TradingSystem::~TradingSystem() {
	try {
		for (const auto &order: m_orders) {
			GetLog().Warn(
				"Order with ID %1% (%2%) still be active."
					"Status: %3%, qty: %4%, remaining qty: %5%.",
				order.id,
				order.tradingSystemOrderId,
				order.status,
				order.qty,
				order.remainingQty);
		}
		AssertEq(0, m_orders.size());
	} catch (...) {
		AssertFailNoException();
	}
}

ConnectorContext & Transaq::TradingSystem::GetConnectorContext() {
	return m_connectorContext;
}

bool Transaq::TradingSystem::IsConnected() const {
	return Connector::IsConnected();
}

void Transaq::TradingSystem::CreateConnection(const IniSectionRef &conf) {
	Init();
	try {
		Connector::Connect(conf);
	} catch (const Connector::ConnectException &ex) {
		throw ConnectException(ex.what());
	}
}

OrderId Transaq::TradingSystem::SendSellAtMarketPrice(
		Security &,
		const Currency &,
		const Qty &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendSellAtMarketPrice");
}

OrderId Transaq::TradingSystem::SendSell(
		Security &security,
		const Currency &,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &,
		const OrderStatusUpdateSlot &&callback) {
	const auto &delayMeasurement
		= GetContext().StartTradingSystemTimeMeasurement();
	const auto &symbol = security.GetSymbol();
	OrderId id;
	try {
		id = TradingConnector::SendSellOrder(
			symbol.GetExchange(),
			symbol.GetSymbol(),
			security.DescalePrice(price),
			qty,
			delayMeasurement);
	} catch (const Exception &ex) {
		GetLog().Error("Failed to send order to sell: \"%1%\".", ex.what());
		throw SendingError("Failed to send order to sell");
	}
	RegisterOrder(
		security,
		id,
		qty,
		std::move(callback),
		std::move(delayMeasurement));
	return id;
}

OrderId Transaq::TradingSystem::SendSellAtMarketPriceWithStopPrice(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendSellAtMarketPriceWithStopPrice");
}

OrderId Transaq::TradingSystem::SendSellImmediatelyOrCancel(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendSellImmediatelyOrCancel");
}

OrderId Transaq::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
		Security &,
		const Currency &,
		const Qty &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq::TradingSystem"
			"::SendSellAtMarketPriceImmediatelyOrCancel");
}

OrderId Transaq::TradingSystem::SendBuyAtMarketPrice(
		Security &,
		const Currency &,
		const Qty &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendBuyAtMarketPrice");
}

OrderId Transaq::TradingSystem::SendBuy(
		Security &security,
		const Currency &,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &,
		const OrderStatusUpdateSlot &&callback) {
	const auto &delayMeasurement
		= GetContext().StartTradingSystemTimeMeasurement();
	const auto &symbol = security.GetSymbol();
	OrderId id;
	try {
		id = TradingConnector::SendBuyOrder(
			symbol.GetExchange(),
			symbol.GetSymbol(),
			security.DescalePrice(price),
			qty,
			delayMeasurement);
	} catch (const Exception &ex) {
		GetLog().Error("Failed to send order to buy: \"%1%\".", ex.what());
		throw SendingError("Failed to send order to buy");
	}
	RegisterOrder(
		security,
		id,
		qty,
		std::move(callback),
		std::move(delayMeasurement));
	return id;
}

OrderId Transaq::TradingSystem::SendBuyAtMarketPriceWithStopPrice(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendBuyAtMarketPriceWithStopPrice");
}
		
OrderId Transaq::TradingSystem::SendBuyImmediatelyOrCancel(
		Security &,
		const Currency &,
		const Qty &,
		const ScaledPrice &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendBuyImmediatelyOrCancel");
}
		
OrderId Transaq::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
		Security &,
		const Currency &,
		const Qty &,
		const OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError(
		"Has no implementation for"
			" trdk::Interaction::Transaq"
			"::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel");
}

void Transaq::TradingSystem::SendCancelOrder(const OrderId &orderId) {

	const auto &delayMeasurement
		= GetContext().StartTradingSystemTimeMeasurement();

	bool isExistent = false;
	{
		const OrdersLock lock(m_ordersMutex);
		auto &index = m_orders.get<ByOrderId>();
		const auto &order = index.find(orderId);
		isExistent = order != index.cend() && order->remainingQty;
	}
	if (!isExistent) {
		boost::format message(
			"Failed to cancel unknown order %1% as it never existed"
				", already filled, canceled or rejected (local error)");
		message % orderId;
		throw OrderIsUnknown(message.str().c_str());
	}

	bool isOrderFound = false;
	try {
		isOrderFound
			= TradingConnector::SendCancelOrder(orderId, delayMeasurement);
	} catch (const Exception &ex) {
		GetLog().Error("Failed to send order canceling: \"%1%\".", ex.what());
		throw SendingError("Failed to send order canceling");
	}
	if (!isOrderFound) {
		boost::format message(
			"Failed to cancel unknown order %1% as it never existed"
				", already filled, canceled or rejected"
				" (trading system error)");
		message % orderId;
		throw OrderIsUnknown(message.str().c_str());
	}

}

void Transaq::TradingSystem::RegisterOrder(
		const Security &security,
		const OrderId &id,
		const Qty &qty,
		const OrderStatusUpdateSlot &&callback,
		const Milestones &&delayMeasurement) {
	const Order order{
		&security,
		id,
		std::move(callback),
		qty,
		qty,
		ORDER_STATUS_SENT,
		std::move(delayMeasurement)
	};
	const OrdersLock lock(m_ordersMutex);
	Verify(m_orders.emplace(std::move(order)).second);
}

void Transaq::TradingSystem::OnOrderUpdate(
		const OrderId &id,
		TradingSystemOrderId &&tradingSystemOrderId,
		OrderStatus &&status,
		Qty &&remainingQty,
		std::string &&tradingSystemMessage,
		const Milestones::TimePoint &replyTime) {

	if (!tradingSystemMessage.empty()) {
		GetLog().Warn(
			"Order %1% (%2%, status %3%, remaining qty %4%) has message"
				" from the trading system: \"%5%\".",
			id,
			tradingSystemOrderId,
			status,
			remainingQty,
			tradingSystemMessage);
	}

	OrderStatusUpdateSlot callback;
	boost::optional<Milestones> replyDelayMeasurement;
	{
	
		const OrdersLock lock(m_ordersMutex);

		auto &index = m_orders.get<ByOrderId>();
		const auto &order = index.find(id);
		if (order == index.cend()) {
			GetTradingLog().Write(
				"unknown order: id=%1%"
					", ts-id=%2%, status=%3%, remaining-qty=%4%"
					", message=\"%5%\"",
				[&](TradingRecord &record) {
					record
						% id
						% tradingSystemOrderId
						% status
						% remainingQty
						% tradingSystemMessage;
				});
			return;
		}

		AssertGe(order->remainingQty, remainingQty);
		Assert(status != ORDER_STATUS_FILLED || remainingQty == 0);

		if (order->tradingSystemOrderId != tradingSystemOrderId) {
			AssertEq(0, order->tradingSystemOrderId);
			AssertEq(ORDER_STATUS_SENT, order->status);
			order->replyDelayMeasurement.Add(
				TSM_ORDER_REPLY_RECEIVED,
				replyTime);
			index.modify(
				order,
				[&tradingSystemOrderId](Order &order) {
					order.tradingSystemOrderId = tradingSystemOrderId;
				});
		}

		if (order->status == ORDER_STATUS_SENT) {
			replyDelayMeasurement = order->replyDelayMeasurement;
		}

		static_assert(numberOfOrderStatuses == 9, "List changed.");
		switch (status) {
			default:
				AssertEq(ORDER_STATUS_SUBMITTED, status);
			case ORDER_STATUS_REJECTED:
			case ORDER_STATUS_INACTIVE:
			case ORDER_STATUS_ERROR:
			case ORDER_STATUS_CANCELLED:
				Assert(
					order->status == ORDER_STATUS_SUBMITTED
					|| order->status == ORDER_STATUS_SENT);
				callback = std::move(order->callback);
				m_orders.erase(order);
				break;
			case ORDER_STATUS_SUBMITTED:
				Assert(
					order->status == ORDER_STATUS_SENT
					|| order->status == ORDER_STATUS_SUBMITTED);
				if (order->status == status) {
					return;
				}
				remainingQty = order->qty;
				order->status = std::move(status);
				callback = order->callback;
				break;
			case ORDER_STATUS_FILLED:
				AssertEq(0, remainingQty);
				Assert(
					order->status == ORDER_STATUS_SENT
					|| order->status == ORDER_STATUS_SUBMITTED);
				if (order->remainingQty) {
					// Will be removed by trade.
					order->status = std::move(status);
				} else {
					m_orders.erase(order);
				}
			case ORDER_STATUS_FILLED_PARTIALLY:
				// Will be notified by trade.
				if (replyDelayMeasurement) {
					replyDelayMeasurement->Measure(TSM_ORDER_REPLY_PROCESSED);
				}
				return;
		}

	}

	Assert(callback);
	callback(
		id,
		boost::lexical_cast<std::string>(tradingSystemOrderId),
		status,
		remainingQty,
		nullptr);

	if (replyDelayMeasurement) {
		replyDelayMeasurement->Measure(TSM_ORDER_REPLY_PROCESSED);
	}

}

void Transaq::TradingSystem::OnTrade(
		std::string &&id,
		TradingSystemOrderId &&tradingSystemOrderId,
		double price,
		Qty &&qty) {

	Order order;
	{
	
		const OrdersLock lock(m_ordersMutex);

		auto &index = m_orders.get<ByTradingSystemOrderId>();
		const auto &it = index.find(tradingSystemOrderId);
		if (it == index.cend()) {
			GetTradingLog().Write(
				"unknown trade: id=%1%, ts-order-id=%2%, price=%3%, qty=%4%",
				[&](TradingRecord &record) {
					record
						% id
						% tradingSystemOrderId
						% price
						% qty;
				});
			return;
		}

		Assert(
			it->status == ORDER_STATUS_SUBMITTED
			|| it->status == ORDER_STATUS_FILLED);

		AssertGe(it->remainingQty, qty);
		it->remainingQty -= qty;

		order = *it;

		if (it->status == ORDER_STATUS_FILLED) {
			index.erase(it);
		}

	}

	const TradeInfo trade{
		std::move(id),
		std::move(qty),
		order.security->ScalePrice(price)
	};
	order.callback(
		order.id,
		boost::lexical_cast<std::string>(tradingSystemOrderId),
		order.remainingQty > 0
			?	ORDER_STATUS_FILLED_PARTIALLY
			:	ORDER_STATUS_FILLED,
		order.remainingQty,
		&trade);

}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem>
CreateTradingSystem(
		const TradingMode &mode,
		size_t tradingSystemIndex,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return boost::make_shared<Transaq::TradingSystem>(
		mode,
		tradingSystemIndex,
		context,
		tag,
		conf);
}

////////////////////////////////////////////////////////////////////////////////
