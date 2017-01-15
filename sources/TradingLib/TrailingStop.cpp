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

////////////////////////////////////////////////////////////////////////////////

TrailingStop::Params::Params(
		const Volume &minProfitPerLotToActivate,
		const Volume &minProfitPerLotToClose)
	: m_minProfitPerLotToActivate(minProfitPerLotToActivate)
	, m_minProfitPerLotToClose(minProfitPerLotToClose) {
	if (m_minProfitPerLotToActivate < m_minProfitPerLotToClose) {
 		throw Exception(
 			"Min profit to activate trailing stop must be greater than"
 				" min profit to close position by trailing stop");
	}
}

const Volume & TrailingStop::Params::GetMinProfitPerLotToActivate() const {
	return m_minProfitPerLotToActivate;
}

const Volume & TrailingStop::Params::GetMinProfitPerLotToClose() const {
	return m_minProfitPerLotToClose;
}

////////////////////////////////////////////////////////////////////////////////

TrailingStop::TrailingStop(
		const boost::shared_ptr<const Params> &params,
		Position &position)
	: StopOrder(position)
	, m_params(params)
	, m_isActivated(false) {
	//...//
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

	static_assert(numberOfCloseReasons == 12, "List changed.");
	switch (GetPosition().GetCloseReason()) {
		case CLOSE_REASON_STOP_LOSS:
			return;
		case CLOSE_REASON_TRAILING_STOP:
			Assert(m_isActivated);
			break;
		default:
			if (!CheckSignal()) {
				return;
			}
			GetPosition().ResetCloseReason(CLOSE_REASON_TRAILING_STOP);
			break;
	}

	OnHit();

}

bool TrailingStop::CheckSignal() {

	AssertNe(CLOSE_REASON_TRAILING_STOP, GetPosition().GetCloseReason());

	const auto &plannedPnl = GetPosition().GetPlannedPnl();
	if (!Activate(plannedPnl)) {
		Assert(!m_minProfit);
		return false;
	}

	const auto &profitToClose = RoundByScale(
		m_params->GetMinProfitPerLotToClose() * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	if (m_minProfit && plannedPnl >= *m_minProfit) {
		return false;
	}

	const bool isSignal = plannedPnl <= profitToClose;

	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%(%6$.8f*%7$.8f=%8$.8f)"
			"\tbid/ask=%9$.8f/%10$.8f\tpos=%11%",
		[&](TradingRecord &record) {
			record
				% GetName() // 1
				% (isSignal ? "signaling" : "trailing"); // 2
			if (m_minProfit) {
				record % *m_minProfit; // 3
			} else {
				record % "none"; // 3
			}
			record % plannedPnl; // 4
			if (isSignal) {
				record % "<="; // 5
			} else {
				record % '>'; // 5
			}
			record
				% m_params->GetMinProfitPerLotToActivate() // 6
				% GetPosition().GetOpenedQty() // 7
				% profitToClose // 8
				% GetPosition().GetSecurity().GetBidPriceValue() // 9
				% GetPosition().GetSecurity().GetAskPriceValue() // 10
				% GetPosition().GetId(); // 11
		});
	
	m_minProfit = plannedPnl;

	return isSignal;

}

bool TrailingStop::Activate(const trdk::Volume &plannedPnl) {
	
	if (m_isActivated) {
		return true;
	}

	const Double &profitToActivate = RoundByScale(
		m_params->GetMinProfitPerLotToActivate() * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	m_isActivated = plannedPnl >= profitToActivate;

	if (m_maxProfit && plannedPnl <= *m_maxProfit) {
		return m_isActivated;
	}
	
	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%(%6$.8f*%7$.8f=%8$.8f)"
			"\tbid/ask=%9$.8f/%10$.8f\tpos=%11%",
		[&](TradingRecord &record) {
			record
				% GetName() // 1
				% (m_isActivated ? "activating" : "accumulating"); // 2
			if (m_maxProfit) {
				record % *m_maxProfit; // 3
			} else {
				record % "none"; // 3
			}
			record % plannedPnl; // 4
			if (m_isActivated) {
				record % ">="; // 5
			} else {
				record % '<'; // 5
			}
			record
				% m_params->GetMinProfitPerLotToActivate() // 6
				% GetPosition().GetOpenedQty() // 7
				% profitToActivate // 8
				% GetPosition().GetSecurity().GetBidPriceValue() // 9
				% GetPosition().GetSecurity().GetAskPriceValue() // 10
				% GetPosition().GetId(); // 11
		});
	
	m_maxProfit = plannedPnl;

	return m_isActivated;

}

////////////////////////////////////////////////////////////////////////////////
