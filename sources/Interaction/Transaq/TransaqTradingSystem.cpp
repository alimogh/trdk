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
		const std::string &instanceName,
		const IniSectionRef &conf)
	: trdk::TradingSystem(mode, index, context, instanceName)
	, TradingConnector(GetContext(), GetLog(), conf)
	, m_connectorContext(GetContext(), GetLog())
	, m_tradingStarted(false) {
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

	enum {
		IS_EXISTENT_YES,
		IS_EXISTENT_NO,
		IS_EXISTENT_TOO_MANY,
	} isExistent = IS_EXISTENT_YES;
	{
		const OrdersLock lock(m_ordersMutex);
		Assert(m_tradingStarted);
		auto &index = m_orders.get<ByOrderId>();
		const auto &order = index.find(orderId);
		static_assert(numberOfOrderStatuses == 9, "List changed.");
		if (
				order == index.cend()
				|| order->remainingQty == 0
				|| !(
					order->status == ORDER_STATUS_SUBMITTED
					|| order->status == ORDER_STATUS_SENT
					|| order->status == ORDER_STATUS_FILLED_PARTIALLY
					|| order->status == ORDER_STATUS_INACTIVE)) {
			isExistent
				= ++m_unknownRequests[orderId] > 100
					|| m_unknownRequests.size() > 100
				?	IS_EXISTENT_TOO_MANY
				:	IS_EXISTENT_NO;
		}
	}
	if (isExistent != IS_EXISTENT_YES) {
		boost::format message(
			"Failed to cancel unknown order %1% as it never existed"
				", already filled, canceled or rejected (local error)");
		message % orderId;
		if (isExistent != IS_EXISTENT_TOO_MANY) {
			throw UnknownOrderCancelError(message.str().c_str());
		} else {
			GetLog().Error("Too many attempts to cancel unknown order.");
			throw Error(message.str().c_str());
		}
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
		throw UnknownOrderCancelError(message.str().c_str());
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
		qty,
		ORDER_STATUS_SENT,
		std::move(delayMeasurement)
	};
	const OrdersLock lock(m_ordersMutex);
	Verify(m_orders.emplace(std::move(order)).second);
	m_tradingStarted = true;
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
			if (!m_tradingStarted) {
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
			} else {
				GetLog().Error(
					"Unknown order received from TRANSAQ server: id=%1%"
						", ts-id=%2%, status=%3%, remaining-qty=%4%"
						", message=\"%5%\".",
					id,
					tradingSystemOrderId,
					status,
					remainingQty,
					tradingSystemMessage);
			}
			return;
		}

		AssertGe(order->remainingQty, remainingQty);
		Assert(status != ORDER_STATUS_FILLED || remainingQty == 0);
		
		if (order->orderBalance < remainingQty) {
			GetLog().Error(
				"Protocol error: order %1% has balance %2%"
					", but new order %3% balance is greater.",
				id,
				order->orderBalance,
				remainingQty);
			throw Exception("TRANSAQ Connector protocol error");
		}
		order->orderBalance = remainingQty;

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
			default:
				AssertEq(ORDER_STATUS_SUBMITTED, status);
				throw Error("Unknown order status from TRANSAQ Connector");
			case ORDER_STATUS_REJECTED:
			case ORDER_STATUS_INACTIVE:
			case ORDER_STATUS_ERROR:
			case ORDER_STATUS_CANCELLED:
				Assert(
					order->status == ORDER_STATUS_SENT
					|| order->status == ORDER_STATUS_SUBMITTED);
				if (order->remainingQty > order->orderBalance) {
					order->status = std::move(status);
					return;
				}
				callback = order->callback;
				m_orders.erase(order);
				break;
			case ORDER_STATUS_FILLED:
				Assert(
					order->status == ORDER_STATUS_SENT
					|| order->status == ORDER_STATUS_SUBMITTED);
				if (order->remainingQty > order->orderBalance) {
					// Will be removed by trade or already did.
					order->status = std::move(status);
				} else {
					m_orders.erase(order);
				}
			case ORDER_STATUS_FILLED_PARTIALLY:
				// Will be notified by trade or already did.
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
	OrderStatus status;
	{
	
		const OrdersLock lock(m_ordersMutex);

		auto &index = m_orders.get<ByTradingSystemOrderId>();
		const auto &it = index.find(tradingSystemOrderId);
		if (it == index.cend()) {
			if (!m_tradingStarted) {
				GetTradingLog().Write(
					"unknown trade"
						": id=%1%, ts-order-id=%2%, price=%3%, qty=%4%",
					[&](TradingRecord &record) {
						record
							% id
							% tradingSystemOrderId
							% price
							% qty;
					});
			} else {
				GetLog().Error(
					"Unknown trade received from TRANSAQ server"
						": id=%1%, ts-order-id=%2%, price=%3%, qty=%4%.",
					id,
					tradingSystemOrderId,
					price,
					qty);
			}
			return;
		}

		Assert(
			it->status == ORDER_STATUS_SUBMITTED
			|| it->status == ORDER_STATUS_FILLED
			|| it->status == ORDER_STATUS_REJECTED
			|| it->status == ORDER_STATUS_INACTIVE
			|| it->status == ORDER_STATUS_ERROR
			|| it->status == ORDER_STATUS_CANCELLED);

		if (it->remainingQty < qty) {
			GetLog().Error(
				"Protocol error: trade has quantity %1%"
					", but order has remaining quantity %2%.",
				qty,
				it->remainingQty);
			throw Exception("Wrong trade quantity");
		}
		AssertGe(it->remainingQty, qty);
		it->remainingQty -= qty;

		order = *it;

		static_assert(numberOfOrderStatuses == 9, "List changed.");
		switch (it->status) {
			case ORDER_STATUS_FILLED:
			case ORDER_STATUS_REJECTED:
			case ORDER_STATUS_INACTIVE:
			case ORDER_STATUS_ERROR:
			case ORDER_STATUS_CANCELLED:
				AssertGe(it->remainingQty, it->orderBalance);
				if (it->remainingQty <= it->orderBalance) {
					status = it->status;
					index.erase(it);
				} else {
					status = ORDER_STATUS_FILLED_PARTIALLY;
				}
				break;
			default:
				status = it->remainingQty
					?	ORDER_STATUS_FILLED_PARTIALLY
					:	ORDER_STATUS_FILLED;
				break;
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
		status,
		order.remainingQty,
		&trade);

}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem>
CreateTradingSystem(
		const TradingMode &mode,
		size_t tradingSystemIndex,
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &conf) {
	return boost::make_shared<Transaq::TradingSystem>(
		mode,
		tradingSystemIndex,
		context,
		instanceName,
		conf);
}

////////////////////////////////////////////////////////////////////////////////
