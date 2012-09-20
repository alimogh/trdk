/**************************************************************************
 *   Created: 2012/09/13 00:13:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "FeedHandler.hpp"
#include "Security.hpp"

using namespace Trader::Interaction::Enyx;

////////////////////////////////////////////////////////////////////////////////

FeedHandler::~FeedHandler() {
#	ifdef DEV_VER
		foreach (const auto &order, m_orders) {
			Assert(!order.second.IsExecuted());
		}
#	endif
}

namespace {

	bool IsBuy(const NXFeed_side_t &side) {
		switch (side) {
			case NXFEED_SIDE_BUY:
				return true;
			default:
				AssertFail("Unknown NXFeed_side_t.");
			case NXFEED_SIDE_SELL:
				return false;
		}
	}

	boost::posix_time::ptime GetMarketDataTimestamp(const NXFeedMessage &meesage) {
		return boost::posix_time::from_time_t(
			const_cast<NXFeedMessage &>(meesage).getMarketDataTimestamp());
	}

}

void FeedHandler::HandleOrderAdd(const nasdaqustvitch41::NXFeedOrderAdd &enyxOrder) {

	const auto &index = m_marketDataSnapshots.get<BySecurtiy>();
	std::string symbol = enyxOrder.getInstrumentName().c_str();
	boost::trim(symbol);
	const auto snapshot = index.find(symbol);
	Assert(snapshot != index.end());
	if (snapshot == index.end()) {
		Log::Warn(
			TRADER_ENYX_LOG_PREFFIX "no Marked Data Snapshot for \"%1%\".",
			symbol);
		return;
	}

	const auto id = enyxOrder.getOrderId();
	const Order order(
		IsBuy(enyxOrder.getBuyNSell()),
		enyxOrder.getQuantity(),
		enyxOrder.getPrice(),
		snapshot->ptr);
	Assert(m_orders.find(id) == m_orders.end());
	m_orders[id] = order;

	order.GetMarketDataSnapshot().AddOrder(
		order.IsBuy(),
		GetMarketDataTimestamp(enyxOrder),
		order.GetQty(),
		order.GetPrice());

}

void FeedHandler::HandleOrderExec(const NXFeedOrderExecute &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	try {
		Order &order = FindOrder(id);
		const auto prevQty = order.GetQty();
		order.ReduceQty(execQty);
		order.MarkAsExecuted();
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			id,
			GetMarketDataTimestamp(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} catch (const OrderNotFoundError &) {
//   		Log::Error(
//   			TRADER_ENYX_LOG_PREFFIX "failed to find EXECUTED order with id %1%.",
//   			id);
	}
}

void FeedHandler::HandleOrderExec(
			const nasdaqustvitch41::NXFeedOrderExeWithPrice &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	try {
		Order &order = FindOrder(id);
		const auto prevQty = order.GetQty();
		order.ReduceQty(execQty);
		order.MarkAsExecuted();
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			id,
			GetMarketDataTimestamp(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} catch (const OrderNotFoundError &) {
//   		Log::Error(
//   			TRADER_ENYX_LOG_PREFFIX "failed to find EXECUTED with PRICE order with id %1%.",
//   			id);
	}
}

void FeedHandler::HandleOrderChange(const NXFeedOrderReduce &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto reducedQty = enyxOrder.getReducedQuantity();
	try {
		Order &order = FindOrder(id);
		const auto prevQty = order.GetQty();
		order.ReduceQty(reducedQty);
		order.GetMarketDataSnapshot().ChangeOrder(
			order.IsBuy(),
			id,
			GetMarketDataTimestamp(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} catch (const OrderNotFoundError &) {
//   		Log::Error(
//   			TRADER_ENYX_LOG_PREFFIX "failed to find CHANGED order with id %1%.",
//   			id);
	}
}

void FeedHandler::HandleOrderDel(const NXFeedOrderDelete &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	try {
		const auto orderIt = FindOrderPos(id);
		const Order order = orderIt->second;
		m_orders.erase(orderIt);
		order.GetMarketDataSnapshot().DelOrder(
			order.IsBuy(),
			id,
			GetMarketDataTimestamp(enyxOrder),
			order.GetQty(),
			order.GetPrice());
	} catch (const OrderNotFoundError &) {
//   		Log::Error(
//   			TRADER_ENYX_LOG_PREFFIX "failed to find DELETED order with id %1%.",
//   			id);
	}
}

FeedHandler::Order & FeedHandler::FindOrder(EnyxOrderId id) {
	return FindOrderPos(id)->second;
}

FeedHandler::Orders::iterator FeedHandler::FindOrderPos(EnyxOrderId id) {
	const auto result = m_orders.find(id);
	Assert(result != m_orders.end());
	if (result == m_orders.end()) {
		throw OrderNotFoundError();
	}
	return result;
}

void FeedHandler::onOrderMessage(NXFeedOrder *orderMsg) {
	try {
		switch(orderMsg->getSubType()) {
			case  NXFEED_SUBTYPE_ORDER_ADD:
				HandleOrderAdd(
					*reinterpret_cast<nasdaqustvitch41::NXFeedOrderAdd *>(
						orderMsg));
				break;
			case  NXFEED_SUBTYPE_ORDER_EXEC:
				HandleOrderExec(
					*reinterpret_cast<NXFeedOrderExecute *>(orderMsg));
				break;
			case  NXFEED_SUBTYPE_ORDER_EXEC_PRICE:
				HandleOrderExec(
					*reinterpret_cast<nasdaqustvitch41::NXFeedOrderExeWithPrice *>(
						orderMsg));
				break;
			case  NXFEED_SUBTYPE_ORDER_REDUCE:
				HandleOrderChange(*reinterpret_cast<NXFeedOrderReduce *>(orderMsg));
				return;
			case  NXFEED_SUBTYPE_ORDER_DEL:
				HandleOrderDel(*reinterpret_cast<NXFeedOrderDelete *>(orderMsg));
				break;
			case  NXFEED_SUBTYPE_ORDER_REPLACE:
				// NXFeedOrderReplace *
			default:
//				Log::Error(TRADER_ENYX_LOG_PREFFIX "unknown message %1%.", orderMsg->getSubType());
				break;
		}
	} catch (...) {
		AssertFailNoException();
	}
}

void FeedHandler::Subscribe(
			const boost::shared_ptr<Security> &security)
		const
		throw() {
	try {
		auto &index = m_marketDataSnapshots.get<BySecurtiy>();
		const auto &pos = index.find(security->GetSymbol());
		if (pos == index.end()) {
			boost::shared_ptr<MarketDataSnapshot> newSnapshot(
				new MarketDataSnapshot(security->GetSymbol()));
			newSnapshot->Subscribe(security);
			m_marketDataSnapshots.insert(MarketDataSnapshotHolder(newSnapshot));
		} else {
			pos->ptr->Subscribe(security);
		}
	} catch (...) {
		AssertFailNoException();
	}
}
