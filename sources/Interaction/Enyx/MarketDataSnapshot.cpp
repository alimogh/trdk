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

void MarketDataSnapshot::AddOrder(bool isBuy, Qty qty, double price) {
	ScaledPrice scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		m_bid[scaledPrice] += qty;
		const auto first = m_bid.begin();
// 		if (first->first != scaledPrice) {
// 			return;
// 		}
		m_security->SetBid(boost::get_system_time(), scaledPrice, qty);
	} else {
		m_ask[scaledPrice] += qty;
		const auto first = m_ask.begin();
// 		if (first->first != scaledPrice) {
// 			return;
// 		}
		m_security->SetAsk(boost::get_system_time(), scaledPrice, qty);
	}
}

void MarketDataSnapshot::ExecOrder(
			bool /*isBuy*/,
			Qty /*prevQty*/,
			Qty /*newQty*/,
			double /*price*/) {
	//...//
}

void MarketDataSnapshot::ChangeOrder(
			bool /*isBuy*/,
			Qty /*prevQty*/,
			Qty /*newQty*/,
			double /*prevPrice*/,
			double /*newPrice*/) {
	//...//
}

void MarketDataSnapshot::DelOrder(bool isBuy, Qty, double price) {
	ScaledPrice scaledPrice = m_security->ScalePrice(price);
	if (isBuy) {
		m_bid.erase(scaledPrice);
		const auto first = m_bid.begin();
		m_security->SetBid(boost::get_system_time(), first->first, first->second);
	} else {
		m_ask.erase(scaledPrice);
		const auto first = m_ask.begin();
		m_security->SetAsk(boost::get_system_time(), first->first, first->second);
	}
}
