/**************************************************************************
 *   Created: 2016/12/18 17:13:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TakeProfit.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

TakeProfit::TakeProfit(
		double minProfitPerLotToActivate,
		double minProfitRatioToClose,
		Position &position)
	: StopOrder(position)
	, m_minProfitPerLotToActivate(minProfitPerLotToActivate)
	, m_minProfitRatioToClose(minProfitRatioToClose)
	, m_isActivated(false) {
	if (m_minProfitRatioToClose > 1) {
		throw Exception(
			"Min profit ratio to close position by take profit"
				" must be less than or equal to 1.0");
	}
}

TakeProfit::~TakeProfit() {
	//...//
}

const char * TakeProfit::GetName() const {
	return "take profit";
}

void TakeProfit::Run() {

	if (!GetPosition().IsOpened()) {
		return;
	}

	static_assert(numberOfCloseTypes == 12, "List changed.");
	switch (GetPosition().GetCloseType()) {
		case CLOSE_TYPE_NONE:
			if (!CheckSignal()) {
				return;
			}
			GetPosition().SetCloseType(CLOSE_TYPE_TAKE_PROFIT);
			break;
		case CLOSE_TYPE_TAKE_PROFIT:
			break;
		default:
			return;
	}

	OnHit();

}

bool TakeProfit::CheckSignal() {

	const auto &plannedPnl = GetPosition().GetPlannedPnl();
	if (!Activate(plannedPnl)) {
		Assert(!m_minProfit);
		return false;
	}
	Assert(m_maxProfit);

	const auto &profitToClose = RoundByScale(
		m_minProfitRatioToClose * *m_maxProfit,
		GetPosition().GetSecurity().GetPriceScale());

	if (m_minProfit && plannedPnl >= *m_minProfit) {
		return false;
	}

	const bool isSignal = plannedPnl <= profitToClose;

	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%(%6$.8f*%7$.2f=%8$.8f)"
			"\tbid/ask=%9$.8f/%10$.8f\tpos=%11%",
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
				% *m_maxProfit
				% m_minProfitRatioToClose
				% profitToClose
				% GetPosition().GetSecurity().GetBidPriceValue()
				% GetPosition().GetSecurity().GetAskPriceValue()
				% GetPosition().GetId();
		});
	
	m_minProfit = plannedPnl;

	return isSignal;

}

bool TakeProfit::Activate(const trdk::Volume &plannedPnl) {

	const Double &profitToActivate = RoundByScale(
		m_minProfitPerLotToActivate * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	bool isSignal = false;
	if (!m_isActivated) {
		m_isActivated = plannedPnl >= profitToActivate;
		if (m_maxProfit && plannedPnl <= *m_maxProfit) {
			Assert(!m_isActivated);
			return false;
		}
		isSignal = m_isActivated;
	} else {
		Assert(m_maxProfit);
		if (plannedPnl <= *m_maxProfit) {
			return m_isActivated;
		}
	}

	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%%6$.8f\tmin-profit=%7$.8f"
			"\tbid/ask=%8$.8f/%9$.8f\tpos=%10%",
		[&](TradingRecord &record) {
			record
				% GetName()
				% (isSignal ? "activating" : "accumulating");
			if (m_maxProfit) {
				record % *m_maxProfit;
			} else {
				record % "none";
			}
			record % plannedPnl;
			if (m_isActivated) {
				record % ">=";
			} else {
				record % '<';
			}
			record % profitToActivate;
			if (m_minProfit) {
				record % *m_minProfit;
			} else {
				record % "none";
			}
			record
				% GetPosition().GetSecurity().GetBidPriceValue()
				% GetPosition().GetSecurity().GetAskPriceValue()
				% GetPosition().GetId();
		});
	
	m_maxProfit = plannedPnl;
	m_minProfit.reset();

	return m_isActivated;

}
