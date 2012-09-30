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

MarketDataSnapshot::MarketDataSnapshot(
			const std::string &symbol,
			bool handlFirstLimitUpdate)
		: m_handlFirstLimitUpdate(handlFirstLimitUpdate),
		m_symbol(symbol) {
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
		if (pos == snapshot.end()) {
			return false;
		}
		const auto deltaQty = prevQty + newQty;
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
			const boost::posix_time::ptime &time,
			Qty prevQty,
			Qty newQty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	AssertLe(newQty, prevQty);
	const auto orderQty = prevQty - newQty;
	if (m_handlFirstLimitUpdate) {
		m_security->SignalNewTrade(time, isBuy, scaledPrice, orderQty);
	}
	if (isBuy) {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_bid)) {
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
			const boost::posix_time::ptime &time,
			Qty prevQty,
			Qty newQty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_bid)) {
			return;
		} else if (m_bid.empty()) {
			m_security->SetBid(time, 0, 0);
		} else {
			UpdateBid(time);
		}
	} else {
		if (!::ChangeOrder(prevQty, newQty, scaledPrice, m_ask)) {
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

void MarketDataSnapshot::ChangeOrder(
			bool isBuy,
			const boost::posix_time::ptime &time,
			Qty prevQty,
			Qty newQty,
			double prevPrice,
			double newPrice) {

	if (Util::IsEqual(prevPrice, newPrice)) {
		ChangeOrder(isBuy, time, prevQty, newQty, newPrice);
		return;
	}

	const auto scaledPrevPrice = m_security->ScalePrice(prevPrice);
	const auto scaledNewPrice = m_security->ScalePrice(newPrice);

	if (isBuy) {
		::DelOrder(prevQty, scaledPrevPrice, m_bid);
		m_bid[scaledNewPrice] += newQty;
		UpdateBid(time);
	} else {
		::DelOrder(prevQty, scaledPrevPrice, m_ask);
		m_ask[scaledNewPrice] += newQty;
		UpdateAsk(time);
	}

}

void MarketDataSnapshot::DelOrder(
			bool isBuy,
			const boost::posix_time::ptime &time,
			Qty qty,
			double price) {
	const auto scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		if (!::DelOrder(qty, scaledPrice, m_bid)) {
			return;
		} else if (m_bid.empty()) {
			m_security->SetBid(time, 0, 0);
		} else {
			UpdateBid(time);
		}
	} else {
		if (!::DelOrder(qty, scaledPrice, m_ask)) {
			return;
		} else if (m_ask.empty()) {
			m_security->SetAsk(time, 0, 0);
		} else {
			UpdateAsk(time);
		}
	}
}
