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

////////////////////////////////////////////////////////////////////////////////

TakeProfit::Params::Params(
		const Volume &minProfitPerLotToActivate,
		const Volume &maxPriceOffsetPerLotToClose)
	: m_minProfitPerLotToActivate(minProfitPerLotToActivate)
	, m_maxPriceOffsetPerLotToClose(maxPriceOffsetPerLotToClose) {
	//...//
}

const Volume & TakeProfit::Params::GetMinProfitPerLotToActivate() const {
	return m_minProfitPerLotToActivate;
}

const Volume & TakeProfit::Params::GetMaxPriceOffsetPerLotToClose() const {
	return m_maxPriceOffsetPerLotToClose;
}

////////////////////////////////////////////////////////////////////////////////

TakeProfit::TakeProfit(
		const boost::shared_ptr<const Params> &params,
		Position &position)
	: StopOrder(position)
	, m_params(params)
	, m_isActivated(false) {
	Assert(m_params);
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

	auto profitToClose = *m_maxProfit;
	profitToClose -= RoundByScale(
		m_params->GetMaxPriceOffsetPerLotToClose()
			* GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	if (m_minProfit && plannedPnl >= *m_minProfit) {
		return false;
	}

	const bool isSignal = plannedPnl <= profitToClose;

	GetTradingLog().Write(
		"%1%\t%2%"
			"\tprofit=%3$.8f->%4$.8f%5%(%6$.8f-%7$.8f*%8$.8f=%9$.8f)"
			"\tbid/ask=%10$.8f/%11$.8f\tpos=%12%",
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
				% *m_maxProfit // 6
				% m_params->GetMaxPriceOffsetPerLotToClose() // 7
				% GetPosition().GetOpenedQty() // 8
				% profitToClose // 9
				% GetPosition().GetSecurity().GetBidPriceValue() // 10
				% GetPosition().GetSecurity().GetAskPriceValue() // 11
				% GetPosition().GetId(); // 12
		});
	
	m_minProfit = plannedPnl;

	return isSignal;

}

bool TakeProfit::Activate(const trdk::Volume &plannedPnl) {

	const Double &profitToActivate = RoundByScale(
		m_params->GetMinProfitPerLotToActivate() * GetPosition().GetOpenedQty(),
		GetPosition().GetSecurity().GetPriceScale());

	bool isSignal = false;
	if (!m_isActivated) {
		m_isActivated = plannedPnl >= profitToActivate;
		if (!m_isActivated && m_maxProfit && plannedPnl <= *m_maxProfit) {
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
