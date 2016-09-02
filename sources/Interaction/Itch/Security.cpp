/**************************************************************************
 *   Created: 2015/03/19 00:44:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Itch;

namespace pt = boost::posix_time;

Itch::Security::Security(
		Context &context,
		const Lib::Symbol &symbol,
		const MarketDataSource &source)
	: Base(context, symbol, source, true, SupportedLevel1Types().set())
	, m_hasBidUpdates(false)
	, m_hasAskUpdates(false)
	, m_maxNewOrderId(0) {
	StartLevel1();
}

void Itch::Security::OnNewOrder(
		const pt::ptime &time,
		bool isBuy,
		const Itch::OrderId &orderId,
		double price,
		double amount) {

	const Order order = {time, isBuy, price, Qty(amount)};
	if (!m_orderBook.emplace(orderId, std::move(order)).second) {
		GetSource().GetLog().Error(
			"Received order with not unique ID %1% (book size: %2%).",
			orderId,
			m_orderBook.size());
		AssertFail("Received order with not unique ID.");
	} else {
		AssertLt(m_maxNewOrderId, orderId);
		m_maxNewOrderId = orderId;
	}

	IncreaseNumberOfUpdates(isBuy);

}

void Itch::Security::OnOrderModify(
		const pt::ptime &time,
		const Itch::OrderId &orderId,
		double amount) {

	const auto &it = m_orderBook.find(orderId);
	if (it == m_orderBook.cend()) {
		if (orderId >= m_maxNewOrderId && m_maxNewOrderId) {
 			GetSource().GetLog().Warn(
 				"Failed to update order in quote book:"
	 				" filed to find order with ID %1%"
					" (book size: %2%, max. ID: %3%).",
 				orderId,
 				m_orderBook.size(),
				m_maxNewOrderId);
			AssertLt(orderId, m_maxNewOrderId);
		}
		return;
	}
	Order &order = it->second; 
	
	AssertLe(order.time, time);
	if (order.time == time && IsEqual(amount, order.qty)) {
		return;
	}

	order.time = time;
	order.qty = amount;

	if (order.isUsed) {
		IncreaseNumberOfUpdates(order.isBuy);
	}

}

void Itch::Security::OnOrderCancel(
		const pt::ptime &time,
		const Itch::OrderId &orderId) {

	const auto &it = m_orderBook.find(orderId);
	if (it == m_orderBook.end()) {
		if (orderId >= m_maxNewOrderId && m_maxNewOrderId) {
			GetSource().GetLog().Warn(
 				"Failed to delete order from quote book:"
 					" filed to find order with ID %1%"
					" (book size: %2%, max. ID: %3%).",
 				orderId,
 				m_orderBook.size(),
				m_maxNewOrderId);
			AssertLt(orderId, m_maxNewOrderId);
		}
		return;
	}

	AssertLe(it->second.time, time);
	UseUnused(time);
	const bool isBuy = it->second.isBuy;
	const bool isUsed = it->second.isUsed;

	m_orderBook.erase(it);
	if (isUsed) {
		IncreaseNumberOfUpdates(isBuy);
	}

}

void Itch::Security::IncreaseNumberOfUpdates(bool isBuy) throw() {
	(isBuy ? m_hasBidUpdates : m_hasAskUpdates) = true;
}

void Itch::Security::Flush(
		const pt::ptime &time,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	if (!m_hasBidUpdates && !m_hasAskUpdates) {
		return;
	}

	m_snapshot.SetTime(time);
	if (m_hasBidUpdates) {
		m_snapshot.GetBid().Clear();
	}
	if (m_hasAskUpdates) {
		m_snapshot.GetAsk().Clear();
	}

	foreach(auto &l, m_orderBook) {
		Order &order = l.second;
		if (order.isBuy) {
			if (m_hasBidUpdates) {
				order.isUsed = m_snapshot.GetBid().Update(
					order.time,
					order.price,
					order.qty);
			}
		} else if (m_hasAskUpdates) {
			order.isUsed = m_snapshot.GetAsk().Update(
				order.time,
				order.price,
				order.qty);
		}
	}

	SetBook(m_snapshot, timeMeasurement);

	m_hasBidUpdates
		= m_hasAskUpdates
		= false;

}

void Itch::Security::ClearBook() {
	m_orderBook.clear();
	m_hasBidUpdates
		= m_hasAskUpdates
		= true;
}
