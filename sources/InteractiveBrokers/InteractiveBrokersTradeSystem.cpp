/**************************************************************************
 *   Created: May 26, 2012 9:44:52 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include "Prec.hpp"
#include "InteractiveBrokersTradeSystem.hpp"
#include "InteractiveBrokersClient.hpp"
#include "Core/Security.hpp"

InteractiveBrokersTradeSystem::InteractiveBrokersTradeSystem()
		: m_client(nullptr) {
	//...//
}

InteractiveBrokersTradeSystem::~InteractiveBrokersTradeSystem() {
	delete m_client;
}

const char * InteractiveBrokersTradeSystem::GetStringStatus(OrderStatus code) const {
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

void InteractiveBrokersTradeSystem::Connect() {
	Assert(!m_client);
	if (m_client) {
		return;
	}
	std::auto_ptr<InteractiveBrokersClient> client(new InteractiveBrokersClient);
	client->Subscribe(
		[&](
					TradeSystem::OrderId id,
					TradeSystem::OrderId /*parentId*/,
					TradeSystem::OrderStatus status,
					long filled,
					long remaining,
					double avgFillPrice,
					double lastFillPrice,
					const std::string &/*whyHeld*/,
					InteractiveBrokersClient::CallbackList &callBackList) {
			UseUnused(remaining);
			std::string symb;
			bool isError = true;
			TradeSystem::OrderStatusUpdateSlot callBack;
			{
				const WriteLock lock(m_mutex);
				const auto pos = m_orderToSecurity.find(id);
				if (pos != m_orderToSecurity.end()) {
					switch (status) {
						default:
							break;
						case TradeSystem::ORDER_STATUS_FILLED:
							Assert(filled > 0);
							isError = false;
							/* no break */
						case TradeSystem::ORDER_STATUS_CANCELLED:
						case TradeSystem::ORDER_STATUS_INACTIVE:
						case TradeSystem::ORDER_STATUS_ERROR:
							symb = pos->second.first;
							callBack = pos->second.second;
							m_orderToSecurity.erase(pos);
							if (	status !=  TradeSystem::ORDER_STATUS_FILLED
									|| remaining == 0) {
								auto posSec = m_securityToOrder.find(symb);
								while (	posSec != m_securityToOrder.end()
										&& posSec->first == symb) {
									if (posSec->second == id) {
										m_securityToOrder.erase(posSec);
										break;
									}
									++posSec;
								}
							}
							break;
					}
				}
			}
			if (callBack) {
				callBackList.push_back(
					boost::bind(callBack, id, status, filled, remaining, avgFillPrice, lastFillPrice));
			}
		});
	client->StartData();
	m_client = client.release();
}

bool InteractiveBrokersTradeSystem::IsCompleted(const Security &security) const {
	const ReadLock lock(m_mutex);
	return m_securityToOrder.find(security.GetFullSymbol()) == m_securityToOrder.end();
}

void InteractiveBrokersTradeSystem::CancelAllOrders(const Security &security) {
	std::list<OrderId> ids;
	std::list<std::string> idsStr;
	{
		const ReadLock lock(m_mutex);
		const SecurityToOrder::const_iterator end = m_securityToOrder.end();
		for (	SecurityToOrder::const_iterator i = m_securityToOrder.find(security.GetFullSymbol());
				i != end && i->first == security.GetFullSymbol();
				++i) {
			ids.push_back(i->second);
			idsStr.push_back(boost::lexical_cast<std::string>(i->second));
		}
	}
	Log::Trading(
		"cancel",
		"%1% orders=[%2%]",
		security.GetSymbol(),
		boost::join(idsStr, ","));
	std::for_each(
		ids.begin(),
		ids.end(),
		[this](OrderId orderId) {
			m_client->CancelOrder(orderId);
		});
}

void InteractiveBrokersTradeSystem::Sell(
			const Security &security,
			OrderQty qty,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	const OrderId orderId = m_client->SendBid(security, qty);
	Log::Trading(
		"sell",
		"%2% id=%1% qty=%3% price=market",
		orderId,
		security.GetSymbol(),
		qty);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}


void InteractiveBrokersTradeSystem::Sell(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	const OrderId orderId = m_client->SendBid(security, qty, rawPrice);
	Log::Trading(
		"sell",
		"%2% id=%1% qty=%3% price=%4%",
		orderId,
		security.GetSymbol(),
		qty,
		rawPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::SellAtMarketPrice(
			const Security &security,
			OrderQty qty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot()*/) {
	const auto rawStopPrice = security.Descale(stopPrice);
	const OrderId orderId = m_client->SendBidWithMarketPrice(security, qty, rawStopPrice);
	Log::Trading(
		"sell",
		"%2% id=%1% type=stop qty=%3% price=market stop-price=%4%",
		orderId,
		security.GetSymbol(),
		qty,
		rawStopPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::SellOrCancel(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	const OrderId orderId = m_client->SendIocBid(security, qty, rawPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::Buy(
			const Security &security,
			OrderQty qty,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	const OrderId orderId = m_client->SendAsk(security, qty);
	Log::Trading(
		"buy",
		"%2% id=%1% qty=%3% price=market",
		orderId,
		security.GetSymbol(),
		qty);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::Buy(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	const OrderId orderId = m_client->SendAsk(security, qty, rawPrice);
	Log::Trading(
		"buy",
		"%2% id=%1% qty=%3% price=%4%",
		orderId,
		security.GetSymbol(),
		qty,
		rawPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::BuyAtMarketPrice(
			const Security &security,
			OrderQty qty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot()*/) {
	const auto rawStopPrice = security.Descale(stopPrice);
	const OrderId orderId = m_client->SendAskWithMarketPrice(security, qty, rawStopPrice);
	Log::Trading(
		"buy",
		"%2% id=%1% type=stop qty=%3% price=market stop-price=%4%",
		orderId,
		security.GetSymbol(),
		qty,
		rawStopPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}

void InteractiveBrokersTradeSystem::BuyOrCancel(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	const OrderId orderId = m_client->SendIocAsk(security, qty, rawPrice);
	const WriteLock lock(m_mutex);
	m_securityToOrder.insert(std::make_pair(security.GetFullSymbol(), orderId));
	m_orderToSecurity[orderId]
		= std::make_pair(security.GetFullSymbol(), stateUpdateSlot);
}
