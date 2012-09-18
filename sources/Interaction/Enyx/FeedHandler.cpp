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

FeedHandler::SecuritySubscribtion::SecuritySubscribtion(
			const boost::shared_ptr<Security> &firstSecurity)
		: m_exchange(firstSecurity->GetPrimaryExchange()),
		m_symbol(firstSecurity->GetSymbol()),
		m_buyOrderAddSignal(new OrderAddSignal),
		m_sellOrderAddSignal(new OrderAddSignal),
		m_buyOrderExecSignal(new OrderExecSignal),
		m_sellOrderExecSignal(new OrderExecSignal),
		m_buyOrderChangeSignal(new OrderChangeSignal),
		m_sellOrderChangeSignal(new OrderChangeSignal),
		m_buyOrderDelSignal(new OrderDelSignal),
		m_sellOrderDelSignal(new OrderDelSignal) {
	Subscribe(firstSecurity);
}

const std::string & FeedHandler::SecuritySubscribtion::GetExchange() const {
	return m_exchange;
}

const std::string & FeedHandler::SecuritySubscribtion::GetSymbol() const {
	return m_symbol;
}

void FeedHandler::SecuritySubscribtion::Subscribe(const boost::shared_ptr<Security> &security) {

	try {

		m_buyOrderAddSignal->connect(
			boost::bind(&Security::OnBuyOrderAdd, security, _1, _2, _3));
		m_sellOrderAddSignal->connect(
			boost::bind(&Security::OnSellOrderAdd, security, _1, _2, _3));

		m_buyOrderExecSignal->connect(
			boost::bind(&Security::OnBuyOrderExec, security, _1, _2, _3));
		m_sellOrderExecSignal->connect(
			boost::bind(&Security::OnSellOrderExec, security, _1, _2, _3));

		m_buyOrderDelSignal->connect(
			boost::bind(&Security::OnBuyOrderDel, security, _1, _2, _3));
		m_sellOrderDelSignal->connect(
			boost::bind(&Security::OnSellOrderDel, security, _1, _2, _3));

	} catch (...) {
		AssertFailNoException();
	}

}

//////////////////////////////////////////////////////////////////////////

FeedHandler::SubscribtionUpdate::SubscribtionUpdate(
			const boost::shared_ptr<Security> &security)
		: m_security(security) {
	//...//
}

void FeedHandler::SubscribtionUpdate::operator ()(SecuritySubscribtion &index) {
	index.Subscribe(m_security);
}

////////////////////////////////////////////////////////////////////////////////

FeedHandler::~FeedHandler() {
	//...//
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

}

void FeedHandler::HandleOrderAdd(const nasdaqustvitch41::NXFeedOrderAdd &order) {

	const auto &index = m_subscribtion.get<BySecurtiy>();
	const std::string symbol = order.getInstrumentName();
	const SubscribtionBySecurity::const_iterator subscribtionPos
		= index.find(boost::make_tuple(std::string("NASDAQ-US"), symbol));
	Assert(subscribtionPos != index.end());
	if (subscribtionPos == index.end()) {
		Log::Warn(
			TRADER_ENYX_LOG_PREFFIX "no marked data handler for \"%1%\".",
			symbol);
		return;
	}
	const auto &subscribtion = *subscribtionPos;

	const bool isBuy = order.getBuyNSell();
	const auto id = order.getOrderId();
	const auto qty = order.getQuantity();
	const auto price = order.getPrice();
	Assert(m_orders.get<ByOrder>().find(id) == m_orders.get<ByOrder>().end());
	m_orders.insert(Order(id, isBuy, qty, price, subscribtion));

	subscribtion.OnOrderAdd(isBuy, id, qty, price);

}

void FeedHandler::HandleOrderExec(const NXFeedOrderExecute &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	try {
		auto order = FindOrderPos(id);
		order->DecreaseQty(execQty);
		const bool isBuy = order->IsBuy();
		const auto price = order->GetPrice();
		const auto &subscribtion = order->GetSubscribtion();
		if (order->GetQty() == 0) {
			m_orders.erase(order);
		}
		subscribtion.OnOrderExec(isBuy, id, execQty, price);
	} catch (const OrderNotFoundError &) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX "failed to fined EXECUTED order with id %1%.",
			id);
	}
}

void FeedHandler::HandleOrderExec(
			const nasdaqustvitch41::NXFeedOrderExeWithPrice &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	try {
		auto order = FindOrderPos(id);
		order->DecreaseQty(execQty);
		const bool isBuy = order->IsBuy();
		const auto &subscribtion = order->GetSubscribtion();
		if (order->GetQty() == 0) {
			m_orders.erase(order);
		}
		subscribtion.OnOrderExec(isBuy, id, execQty, enyxOrder.getExecutedPrice());
	} catch (const OrderNotFoundError &) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX "failed to fined EXECUTED order with id %1%.",
			id);
	}
}

void FeedHandler::HandleOrderDel(const NXFeedOrderDelete &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	try {
		auto order = FindOrderPos(id);
		const auto &subscribtion = order->GetSubscribtion();
		const bool isBuy = order->IsBuy();
		const auto qty = order->GetQty();
		const auto price = order->GetPrice();
		m_orders.erase(order);
		subscribtion.OnOrderDel(isBuy, id, qty, price);
	} catch (const OrderNotFoundError &) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX "failed to fined DELETED order with id %1%.",
			id);
	}
}

void FeedHandler::HandleOrderChange(const NXFeedOrderReduce &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto reducedQty = enyxOrder.getReducedQuantity();
	try {
		auto order = FindOrderPos(id);
		const bool isBuy = order->IsBuy();
		const auto qty = order->DecreaseQty(reducedQty);
		const auto price = order->GetPrice();
		const auto &subscribtion = order->GetSubscribtion();
		if (order->GetQty() == 0) {
			m_orders.erase(order);
		}
		subscribtion.OnOrderChange(isBuy, id, qty, price);
	} catch (const OrderNotFoundError &) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX "failed to fined CHANGED order with id %1%.",
			id);
	}
}

const FeedHandler::Order & FeedHandler::FindOrder(Security::OrderId id) {
	const FeedHandler::OrderById::iterator pos = FindOrderPos(id);
	const FeedHandler::Order &order = *pos;
	return order;
}

FeedHandler::OrderById::iterator FeedHandler::FindOrderPos(Security::OrderId id) {
	const auto &index = m_orders.get<ByOrder>();
	const auto orderPos = index.find(id);
	Assert(orderPos != index.end());
	if (orderPos == index.end()) {
		throw OrderNotFoundError();
	}
	AssertEq(id, orderPos->GetOrderId());
	return orderPos;
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
				Log::Error(TRADER_ENYX_LOG_PREFFIX "unknown message %1%.", orderMsg->getSubType());
				break;
		}
	} catch (...) {
		AssertFailNoException();
	}
}

void FeedHandler::Subscribe(const boost::shared_ptr<Security> &security) const throw() {
	try {
		auto &index = m_subscribtion.get<BySecurtiy>();
		const auto &pos = index.find(
			boost::make_tuple(security->GetPrimaryExchange(), security->GetSymbol()));
		if (pos == index.end()) {
			m_subscribtion.insert(SecuritySubscribtion(security));
		} else {
			index.modify(pos, SubscribtionUpdate(security));
		}
	} catch (...) {
		AssertFailNoException();
	}
}
