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
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

StopLoss::StopLoss(double maxLossPerLot, Position &position)
	: StopOrder(position)
	, m_maxLossPerLot(maxLossPerLot) {
	//...//
}

StopLoss::~StopLoss() {
	//...//
}

const char * StopLoss::GetName() const {
	return "stop loss";
}

void StopLoss::Run() {

	if (!GetPosition().IsOpened()) {
		return;
	}

	static_assert(numberOfCloseTypes == 12, "List changed.");
	switch (GetPosition().GetCloseType()) {

		case CLOSE_TYPE_STOP_LOSS:
			break;

		case CLOSE_TYPE_TRAILING_STOP:
			return;

		default:
			{

				const Double maxLoss
					= GetPosition().GetOpenedQty() * -m_maxLossPerLot;
				const auto &plannedPnl = GetPosition().GetPlannedPnl();

				if (maxLoss < plannedPnl) {
					return;
				}

				GetTradingLog().Write(
					"%1%\thit"
						"\tplanned-pnl=%2$.8f\tmax-loss=%3$.8f*%4$.8f=%5$.8f"
						"\tbid/ask=%6$.8f/%7$.8f\tpos=%8%",
					[&](TradingRecord &record) {
						record
							% GetName()
							% plannedPnl
							% GetPosition().GetOpenedQty()
							% m_maxLossPerLot
							% maxLoss
							% GetPosition().GetSecurity().GetBidPriceValue()
							% GetPosition().GetSecurity().GetAskPriceValue()
							% GetPosition().GetId();
					});

				GetPosition().ResetCloseType(CLOSE_TYPE_STOP_LOSS);

			}

	}

	OnHit();

}
