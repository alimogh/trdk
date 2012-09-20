/**************************************************************************
 *   Created: 2012/09/18 23:58:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSnapshot.hpp"

using namespace Trader::Interaction::Enyx;

MarketDataSnapshot::MarketDataSnapshot(const std::string &symbol)
		: m_symbol(symbol) {
	//...//
}

void MarketDataSnapshot::Subscribe(
			const boost::shared_ptr<Security> &security)
		const {
	Assert(!m_security);
	m_security = security;
}

void MarketDataSnapshot::UpdateBid(const boost::posix_time::ptime &time) {
	const Bid::const_iterator begin = m_bid.begin();
	m_security->SetBid(time, begin->first, begin->second);
}

void MarketDataSnapshot::UpdateAsk(const boost::posix_time::ptime &time) {
	const Ask::const_iterator begin = m_ask.begin();
	m_security->SetAsk(time, begin->first, begin->second);
}

void MarketDataSnapshot::AddOrder(
			bool isBuy,
			const boost::posix_time::ptime &time,
			Qty qty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		m_bid[scaledPrice] += qty;
		UpdateBid(time);
	} else {
		m_ask[scaledPrice] += qty;
		UpdateAsk(time);
	}
}

namespace {

	template<typename Snapshot>
	bool ChangeOrder(
				MarketDataSnapshot::Qty prevQty,
				MarketDataSnapshot::Qty newQty,
				MarketDataSnapshot::ScaledPrice price,
				Snapshot &snapshot) {
		const auto pos = snapshot.find(price);
		Assert(pos != snapshot.end());
		Assert(pos->second >= prevQty);
		if (pos == snapshot.end()) {
			return false;
		}
		const auto deltaQty = prevQty + newQty;
		Assert(pos->second > deltaQty);
		if (pos->second <= deltaQty) {
			snapshot.erase(pos);
		} else {
			pos->second -= deltaQty;
		}
		return true;
	}

}

void MarketDataSnapshot::ExecOrder(
			bool isBuy,
			OrderId /*orderId*/,
			const boost::posix_time::ptime &time,
			Qty prevQty,
			Qty newQty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	Assert(newQty <= prevQty);
	const auto orderQty = prevQty - newQty;
	if (isBuy) {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_bid)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to EXEC snapshot buy-order"
					" (order: %1%; symbol: %2%, qty: %3% -> %4%, price: %5%; time: %6%, snapshot size: %7%).",
				orderId,
				m_symbol,
				prevQty,
				newQty,
				price,
				time,
				m_bid.size()); */
			m_security->SetLast(time, scaledPrice, orderQty);
		} else {
			ScaledPrice bestPrice = 0;
			Qty bestQty = 0;
			 if (!m_bid.empty()) {
				 Bid::const_iterator begin = m_bid.begin();
				 bestPrice = begin->first;
				 bestQty = begin->second;
			 }
			m_security->SetLastAndBid(time, scaledPrice, orderQty, bestPrice, bestQty);
		}
	} else {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_ask)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to EXEC snapshot sell-order"
					" (order: %1%; symbol: %2%, qty: %3% -> %4%, price: %5%; time: %6%, snapshot size: %7%).",
				orderId,
				m_symbol,
				prevQty,
				newQty,
				price,
				time,
				m_ask.size()); */
			m_security->SetLast(time, scaledPrice, orderQty);
		} else {
			ScaledPrice bestPrice = 0;
			Qty bestQty = 0;
			 if (!m_ask.empty()) {
				 Ask::const_iterator begin = m_ask.begin();
				 bestPrice = begin->first;
				 bestQty = begin->second;
			 }
			m_security->SetLastAndAsk(time, scaledPrice, orderQty, bestPrice, bestQty);
		}
	}
}

void MarketDataSnapshot::ChangeOrder(
			bool isBuy,
			OrderId /*orderId*/,
			const boost::posix_time::ptime &time,
			Qty prevQty,
			Qty newQty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_bid)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to CHANGE snapshot buy-order"
					" (order: %1%; symbol: %2%, qty: %3% -> %4%, price: %5%; time: %6%, snapshot size: %7%).",
				orderId,
				m_symbol,
				prevQty,
				newQty,
				price,
				time,
				m_bid.size());*/
			return;
		} else if (m_bid.empty()) {
			m_security->SetBid(time, 0, 0);
		} else {
			UpdateBid(time);
		}
	} else {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_ask)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to CHANGE snapshot sell-order"
					" (order: %1%; symbol: %2%, qty: %3% -> %4%, price: %5%; time: %6%, snapshot size: %7%).",
				orderId,
				m_symbol,
				prevQty,
				newQty,
				price,
				time,
				m_ask.size());*/
			return;
		} else if (m_ask.empty()) {
			m_security->SetAsk(time, 0, 0);
		} else {
			UpdateAsk(time);
		}
	}
}

namespace {

	template<typename Snapshot>
	bool DelOrder(
				MarketDataSnapshot::Qty qty,
				MarketDataSnapshot::ScaledPrice price,
				Snapshot &snapshot) {
		const auto pos = snapshot.find(price);
		Assert(pos != snapshot.end());
		Assert(pos->second >= qty);
		if (pos == snapshot.end()) {
			return false;
		}
		if (pos->second <= qty) {
			snapshot.erase(pos);
		} else {
			pos->second -= qty;
		}
		return true;
	}

}

void MarketDataSnapshot::DelOrder(
			bool isBuy,
			OrderId /*orderId*/,
			const boost::posix_time::ptime &time,
			Qty qty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		if (!::DelOrder(qty, scaledPrice, m_bid)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to DELETE buy-order from snapshot"
					" (order: %1%; symbol: %2%, qty: %3%, price: %4%; time: %5%, snapshot size: %6%).",
				orderId,
				m_symbol,
				qty,
				price,
				time,
				m_bid.size());*/
			return;
		} else if (m_bid.empty()) {
			m_security->SetBid(time, 0, 0);
		} else {
			UpdateBid(time);
		}
	} else {
		if (!::DelOrder(qty, scaledPrice, m_ask)) {
/*			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to DELETE sell-order from snapshot"
					" (order: %1%; symbol: %2%, qty: %3%, price: %4%; time: %5%, snapshot size: %6%).",
				orderId,
				m_symbol,
				qty,
				price,
				time,
				m_ask.size());*/
			return;
		} else if (m_ask.empty()) {
			m_security->SetAsk(time, 0, 0);
		} else {
			UpdateAsk(time);
		}
	}
}
