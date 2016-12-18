/**************************************************************************
 *   Created: 2016/12/15 04:18:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TrailingStop.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

TrailingStop::TrailingStop(
		double minProfitPerLotToActivate,
		double minProfitPerLotToClose,
		Position &position)
	: StopOrder(position)
	, m_minProfitPerLotToActivate(minProfitPerLotToActivate)
	, m_minProfitPerLotToClose(minProfitPerLotToClose)
	, m_isActivated(false) {
	if (m_minProfitPerLotToActivate < m_minProfitPerLotToClose) {
		throw Exception(
			"Min profit to activate trailing stop must be greater than"
				" min profit to close position by trailing stop");
	}
}

TrailingStop::~TrailingStop() {
	//...//
}

const char * TrailingStop::GetName() const {
	return "trailing stop";
}

void TrailingStop::Run() {

	if (!GetPosition().IsOpened()) {
		Assert(!m_isActivated);
		return;
	}

	static_assert(numberOfCloseTypes == 12, "List changed.");
	switch (GetPosition().GetCloseType()) {
		case CLOSE_TYPE_STOP_LOSS:
			return;
		case CLOSE_TYPE_TRAILING_STOP:
			Assert(m_isActivated);
			break;
		default:
			if (!CheckSignal()) {
				return;
			}
			GetPosition().ResetCloseType(CLOSE_TYPE_TRAILING_STOP);
			break;
	}

	OnHit();

}

bool TrailingStop::CheckSignal() {

	AssertNe(CLOSE_TYPE_TRAILING_STOP, GetPosition().GetCloseType());

	const auto &plannedPnl = GetPosition().GetPlannedPnl();
	if (!Activate(plannedPnl)) {
		Assert(!m_minProfit);
		return false;
	}

	const auto &profitToClose = RoundByScale(
		m_minProfitPerLotToClose * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	if (m_minProfit && plannedPnl >= *m_minProfit) {
		return false;
	}

	const bool isSignal = plannedPnl <= profitToClose;

	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%%6$.8f"
			"\tbid/ask=%7$.8f/%8$.8f\tpos=%9%",
		[&](TradingRecord &record) {
			record
				% GetName()
				% (isSignal ? "signaling" : "trailing");
			if (m_minProfit) {
				record % *m_minProfit;
			} else {
				record % "none";
			}
			record % plannedPnl;
			if (isSignal) {
				record % "<=";
			} else {
				record % '>';
			}
			record
				% profitToClose
				% GetPosition().GetSecurity().GetBidPriceValue()
				% GetPosition().GetSecurity().GetAskPriceValue()
				% GetPosition().GetId();
		});
	
	m_minProfit = plannedPnl;

	return isSignal;

}

bool TrailingStop::Activate(const trdk::Volume &plannedPnl) {
	
	if (m_isActivated) {
		return true;
	}

	const Double &profitToActivate = RoundByScale(
		m_minProfitPerLotToActivate * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	m_isActivated = plannedPnl >= profitToActivate;

	if (m_maxProfit && plannedPnl <= *m_maxProfit) {
		return m_isActivated;
	}
	
	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%%6$.8f"
			"\tbid/ask=%7$.8f/%8$.8f\tpos=%9%",
		[&](TradingRecord &record) {
			record
				% GetName()
				% (m_isActivated ? "activating" : "accumulating");
			if (m_maxProfit) {
				record % *m_maxProfit;
			} else {
				record % "none";
			}
			record % plannedPnl;
			if (m_isActivated) {
				record % '>=';
			} else {
				record % "<";
			}
			record
				% profitToActivate
				% GetPosition().GetSecurity().GetBidPriceValue()
				% GetPosition().GetSecurity().GetAskPriceValue()
				% GetPosition().GetId();
		});
	
	m_maxProfit = plannedPnl;

	return m_isActivated;

}
