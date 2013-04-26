/**************************************************************************
 *   Created: May 26, 2012 9:44:52 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"
#include "Client.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::InteractiveBrokers;

namespace ib = trdk::Interaction::InteractiveBrokers;


ib::TradeSystem::TradeSystem() {
	//...//
}

ib::TradeSystem::~TradeSystem() {
	//...//
}

void ib::TradeSystem::Connect(const IniFileSectionRef &settings) {

	Assert(!m_client);
	if (m_client) {
		return;
	}

	std::unique_ptr<Client> client(
		new Client(
			settings.ReadTypedKey<int>("client_id", 0),
			settings.ReadKey("ip_address", "127.0.0.1")));
	client->Subscribe(
		[this](
					trdk::OrderId id,
					trdk::OrderId /*parentId*/,
					OrderStatus status,
					long filled,
					long remaining,
					double avgFillPrice,
					double lastFillPrice,
					const std::string &/*whyHeld*/,
					Client::CallbackList &callBackList) {
			OrderStatusUpdateSlot callBack;
			{
				PlacedOrderById &index = m_placedOrders.get<ByOrder>();
				const WriteLock lock(m_mutex);
				const auto pos = index.find(id);
				if (pos == index.end()) {
					return;
				}
				callBack = pos->callback;
				switch (status) {
					default:
						return;
					case ORDER_STATUS_FILLED:
						Assert(filled > 0);
						if (remaining == 0) {
							index.erase(pos);
						}
						break;
					case ORDER_STATUS_CANCELLED:
					case ORDER_STATUS_INACTIVE:
					case ORDER_STATUS_ERROR:
						index.erase(pos);
						break;
				}
			}
			if (!callBack) {
				return;
			}
			callBackList.push_back(
				boost::bind(
					callBack,
					id,
					status,
					filled,
					remaining,
					avgFillPrice,
					lastFillPrice));
		});

	client->StartData();

	client.swap(m_client);

}

void ib::TradeSystem::CancelOrder(trdk::OrderId orderId) {
	m_client->CancelOrder(orderId);
}

void ib::TradeSystem::CancelAllOrders(Security &security) {
	std::list<trdk::OrderId> ids;
	std::list<std::string> idsStr;
	{
		const ReadLock lock(m_mutex);
		const PlacedOrderBySymbol &index = m_placedOrders.get<BySymbol>();
		const auto symbol = security.GetFullSymbol();
		for (	PlacedOrderBySymbol::const_iterator i = index.find(security.GetFullSymbol());
				i != index.end() && i->symbol == symbol;
				++i) {
			ids.push_back(i->id);
			idsStr.push_back(boost::lexical_cast<std::string>(i->id));
		}
	}
	std::for_each(
		ids.begin(),
		ids.end(),
		[this](trdk::OrderId orderId) {
			m_client->CancelOrder(orderId);
		});
}

trdk::OrderId ib::TradeSystem::SellAtMarketPrice(
			Security &security,
			Qty qty,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	PlacedOrder order = {};
	order.id = m_client->SendBid(security, qty);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}


trdk::OrderId ib::TradeSystem::Sell(
			Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->SendBid(security, qty, rawPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SellAtMarketPriceWithStopPrice(
			Security &security,
			Qty qty,
			ScaledPrice stopPrice,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const auto rawStopPrice = security.DescalePrice(stopPrice);
	PlacedOrder order = {};
	order.id = m_client->SendBidWithMarketPrice(security, qty, rawStopPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SellOrCancel(
			Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->SendIocBid(security, qty, rawPrice),
		security.GetFullSymbol(),
		statusUpdateSlot};
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::BuyAtMarketPrice(
			Security &security,
			Qty qty,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	PlacedOrder order = {};
	order.id = m_client->SendAsk(security, qty);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::Buy(
			Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->SendAsk(security, qty, rawPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::BuyAtMarketPriceWithStopPrice(
			Security &security,
			Qty qty,
			ScaledPrice stopPrice,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const auto rawStopPrice = security.DescalePrice(stopPrice);
	PlacedOrder order = {};
	order.id = m_client->SendAskWithMarketPrice(security, qty, rawStopPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::BuyOrCancel(
			Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->SendIocAsk(security, qty, rawPrice),
		security.GetFullSymbol(),
		statusUpdateSlot};
	RegOrder(order);
	return order.id;
}

void ib::TradeSystem::RegOrder(const PlacedOrder &order) {
	const WriteLock lock(m_mutex);
	Assert(
		m_placedOrders.get<ByOrder>().find(order.id)
		== m_placedOrders.get<ByOrder>().end());
	m_placedOrders.insert(order);
}
