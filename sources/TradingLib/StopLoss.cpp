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

StopLoss::StopLoss(const ScaledPrice &maxLossPerQty, Position &position)
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

	const auto maxLoss = m_position.GetOpenedQty() * -m_maxLossPerQty;
	const auto &plannedPnl = m_position.GetPlannedPnl();
	if (plannedPnl >= maxLoss) {
		return;
	}
	
	m_position.GetStrategy().GetTradingLog().Write(
		"stop-loss\tpos=%1%"
			"\tplanned-pnl=%2$.8f$\tmax-loss=%3$.8f$*%4$.8f$=%5$.8f$",
		[&](TradingRecord &record) {
			record
				% m_position.GetId()
				% plannedPnl
				% m_position.GetOpenedQty()
				% m_maxLossPerQty
				% maxLoss;
		});

	OnHit();

}

void StopLoss::OnHit() {
	m_position.HasActiveOrders()
		?	m_position.CancelAllOrders()
		:	m_position.Close(
				Position::CLOSE_TYPE_STOP_LOSS,
				m_position.GetMarketClosePrice());
}

Position & StopLoss::GetPosition() {
	return m_position;
}

const Position & StopLoss::GetPosition() const {
	return const_cast<StopLoss *>(this)->GetPosition();
}
