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

namespace mi = boost::multi_index;

//////////////////////////////////////////////////////////////////////////

namespace {

	typedef boost::shared_mutex Mutex;
	typedef boost::unique_lock<Mutex> WriteLock;
	typedef boost::shared_lock<Mutex> ReadLock;

	struct PlacedOrder {
		TradeSystem::OrderId id;
		std::string symbol;
		TradeSystem::OrderStatusUpdateSlot callback;
		bool comission;
		bool completed;
	};

	struct BySymbol {
		//...//
	};
	struct ByOrder {
		//...//
	};
	struct BySymbolAndOrder {
		//...//
	};

	typedef boost::multi_index_container<
		PlacedOrder,
		mi::indexed_by<
			mi::ordered_non_unique<
				mi::tag<BySymbol>,
				mi::member<PlacedOrder, std::string, &PlacedOrder::symbol>>,
			mi::hashed_unique<
				mi::tag<ByOrder>,
				mi::member<PlacedOrder, TradeSystem::OrderId, &PlacedOrder::id>>>>
		PlacedOrderSet;
	typedef PlacedOrderSet::index<BySymbol>::type PlacedOrderBySymbol;
	typedef PlacedOrderSet::index<ByOrder>::type PlacedOrderById;

}

//////////////////////////////////////////////////////////////////////////

class InteractiveBrokersTradeSystem::Implementation : private boost::noncopyable {

public:

	Mutex mutex;
	std::unique_ptr<InteractiveBrokersClient> client;
	PlacedOrderSet placedOrders;

	void RegOrder(const PlacedOrder &order) {
		const WriteLock lock(mutex);
		Assert(placedOrders.get<ByOrder>().find(order.id) == placedOrders.get<ByOrder>().end());
		placedOrders.insert(order);
	}

};

//////////////////////////////////////////////////////////////////////////

InteractiveBrokersTradeSystem::InteractiveBrokersTradeSystem()
		: m_pimpl(new Implementation) {
	//...//
}

InteractiveBrokersTradeSystem::~InteractiveBrokersTradeSystem() {
	delete m_pimpl;
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
	Assert(!m_pimpl->client);
	if (m_pimpl->client) {
		return;
	}
	std::unique_ptr<InteractiveBrokersClient> client(new InteractiveBrokersClient);
	client->Subscribe(
		[this](
					TradeSystem::OrderId id,
					TradeSystem::OrderId /*parentId*/,
					TradeSystem::OrderStatus status,
					long filled,
					long remaining,
					double avgFillPrice,
					double lastFillPrice,
					const std::string &/*whyHeld*/,
					InteractiveBrokersClient::CallbackList &callBackList) {
			TradeSystem::OrderStatusUpdateSlot callBack;
			{
				PlacedOrderById &index = m_pimpl->placedOrders.get<ByOrder>();
				const WriteLock lock(m_pimpl->mutex);
				const auto pos = index.find(id);
				if (pos == index.end()) {
					return;
				}
				callBack = pos->callback;
				switch (status) {
					default:
						return;
					case TradeSystem::ORDER_STATUS_COMISSION:
// 						if (pos->completed) {
// 							index.erase(pos);
// 						} else if (!pos->comission) {
// 							PlacedOrder order(*pos);
// 							order.comission = true;
// 							index.replace(pos, order);
// 						}
						break;
					case TradeSystem::ORDER_STATUS_FILLED:
						Assert(filled > 0);
						if (remaining == 0) {
							index.erase(pos);
						}
						break;
					case TradeSystem::ORDER_STATUS_CANCELLED:
					case TradeSystem::ORDER_STATUS_INACTIVE:
					case TradeSystem::ORDER_STATUS_ERROR:
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
					lastFillPrice,
					comission));
		});
	client->StartData();
	m_pimpl->client.reset(client.release());
}

bool InteractiveBrokersTradeSystem::IsCompleted(const Security &security) const {
	const ReadLock lock(m_pimpl->mutex);
	const PlacedOrderBySymbol &index = m_pimpl->placedOrders.get<BySymbol>();
	return index.find(security.GetFullSymbol()) == index.end();
}

void InteractiveBrokersTradeSystem::CancelAllOrders(const Security &security) {
	std::list<OrderId> ids;
	std::list<std::string> idsStr;
	{
		const ReadLock lock(m_pimpl->mutex);
		const PlacedOrderBySymbol &index = m_pimpl->placedOrders.get<BySymbol>();
		const auto symbol = security.GetFullSymbol();
		for (	PlacedOrderBySymbol::const_iterator i = index.find(security.GetFullSymbol());
				i != index.end() && i->symbol == symbol;
				++i) {
			ids.push_back(i->id);
			idsStr.push_back(boost::lexical_cast<std::string>(i->id));
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
			m_pimpl->client->CancelOrder(orderId);
		});
}

void InteractiveBrokersTradeSystem::Sell(
			const Security &security,
			OrderQty qty,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendBid(security, qty);
	Log::Trading(
		"sell",
		"%2% id=%1% qty=%3% price=market",
		order.id,
		security.GetSymbol(),
		qty);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}


void InteractiveBrokersTradeSystem::Sell(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendBid(security, qty, rawPrice);
	Log::Trading(
		"sell",
		"%2% id=%1% qty=%3% price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		rawPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::SellAtMarketPrice(
			const Security &security,
			OrderQty qty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot()*/) {
	const auto rawStopPrice = security.Descale(stopPrice);
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendBidWithMarketPrice(security, qty, rawStopPrice);
	Log::Trading(
		"sell",
		"%2% id=%1% type=stop qty=%3% price=market stop-price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		rawStopPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::SellOrCancel(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot() */) {
	const PlacedOrder order = {
		m_pimpl->client->SendIocBid(security, qty, security.Descale(price)),
		security.GetFullSymbol(),
		stateUpdateSlot};
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::Buy(
			const Security &security,
			OrderQty qty,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendAsk(security, qty);
	Log::Trading(
		"buy",
		"%2% id=%1% qty=%3% price=market",
		order.id,
		security.GetSymbol(),
		qty);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::Buy(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	const auto rawPrice = security.Descale(price);
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendAsk(security, qty, rawPrice);
	Log::Trading(
		"buy",
		"%2% id=%1% qty=%3% price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		rawPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::BuyAtMarketPrice(
			const Security &security,
			OrderQty qty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot stateUpdateSlot /* = StateUpdateSlot()*/) {
	const auto rawStopPrice = security.Descale(stopPrice);
	PlacedOrder order = {};
	order.id = m_pimpl->client->SendAskWithMarketPrice(security, qty, rawStopPrice);
	Log::Trading(
		"buy",
		"%2% id=%1% type=stop qty=%3% price=market stop-price=%4%",
		order.id,
		security.GetSymbol(),
		qty,
		rawStopPrice);
	order.symbol = security.GetFullSymbol();
	order.callback = stateUpdateSlot;
	m_pimpl->RegOrder(order);
}

void InteractiveBrokersTradeSystem::BuyOrCancel(
			const Security &security,
			OrderQty qty,
			OrderPrice price,
			OrderStatusUpdateSlot stateUpdateSlot /* =  StateUpdateSlot() */) {
	const PlacedOrder order = {
		m_pimpl->client->SendIocAsk(security, qty, security.Descale(price)),
		security.GetFullSymbol(),
		stateUpdateSlot};
	m_pimpl->RegOrder(order);
}
