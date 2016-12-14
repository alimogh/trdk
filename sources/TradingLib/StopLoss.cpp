/**************************************************************************
 *   Created: 2016/12/14 03:07:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StopLoss.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

StopLoss::StopLoss(double maxLossPerQty, Position &position)
	: m_maxLossPerQty(maxLossPerQty)
	, m_position(position) {
	//...//
}

StopLoss::~StopLoss() {
	//...//
}

void StopLoss::Run() {

	if (!m_position.IsOpened()) {
		return;
	}

	if (m_position.GetCloseType() != CLOSE_TYPE_STOP_LOSS) {
		
		const Qty maxLoss = m_position.GetOpenedQty() * -m_maxLossPerQty;
		const auto &plannedPnl = m_position.GetPlannedPnl();

		if (maxLoss < plannedPnl) {
			return;
		}

		m_position.GetStrategy().GetTradingLog().Write(
			"stop loss\thit"
				"\tplanned-pnl=%1$.8f\tmax-loss=%2$.8f*%3$.8f=%4$.8f"
				"\tbid/ask=%5$.8f/%6$.8f"
				"\tpos=%7%",
			[&](TradingRecord &record) {
				record
					% plannedPnl
					% m_position.GetOpenedQty()
					% m_maxLossPerQty
					% maxLoss
					% m_position.GetSecurity().GetBidPriceValue()
					% m_position.GetSecurity().GetAskPriceValue()
					% m_position.GetId();
			});

		m_position.ResetCloseType(CLOSE_TYPE_STOP_LOSS);

	}

	OnHit();

}

void StopLoss::OnHit() {

	if (m_position.HasActiveOpenOrders()) {
	
		m_position.GetStrategy().GetTradingLog().Write(
			"stop loss\tbad open-order\tpos=%1%",
			[&](TradingRecord &record) {record % m_position.GetId();});
	
		m_position.CancelAllOrders();
	
	} else if (m_position.HasActiveCloseOrders()) {
	
		const auto &orderPrice = m_position.GetActiveCloseOrderPrice();
		const auto &currentPrice = m_position.GetMarketClosePrice();

		const bool isBadOrder = !m_position.IsLong()
			?	orderPrice < currentPrice
			:	orderPrice > currentPrice;
		m_position.GetStrategy().GetTradingLog().Write(
				"stop loss\t%1%\t%2%"
					"\torder-price=%3$.8f\tcurrent-price=%4$.8f\tpos=%5%",
			[&](TradingRecord &record) {
				record
					%	(isBadOrder
							?	"canceling bad close-order"
							:	"close order is good")
					%	m_position.GetCloseOrderSide()
					%	m_position.GetSecurity().DescalePrice(orderPrice)
					%	m_position.GetSecurity().DescalePrice(currentPrice)
					%	m_position.GetId();
			});
		if (isBadOrder) {
			m_position.CancelAllOrders();
		}
	
	} else {
	
		m_position.GetStrategy().GetTradingLog().Write(
			"stop loss\tclosing\tpos=%1%",
			[&](TradingRecord &record) {record % m_position.GetId();});

		m_position.Close(m_position.GetMarketClosePrice());
	
	}

}

Position & StopLoss::GetPosition() {
	return m_position;
}

const Position & StopLoss::GetPosition() const {
	return const_cast<StopLoss *>(this)->GetPosition();
}
