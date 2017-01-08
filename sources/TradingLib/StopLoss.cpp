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

////////////////////////////////////////////////////////////////////////////////

StopLossOrder::StopLossOrder(Position &position)
	: StopOrder(position) {
	//...//
}

StopLossOrder::~StopLossOrder() {
	//...//
}

void StopLossOrder::Run() {

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
			if (!Activate()) {
				return;
			}
			GetPosition().ResetCloseType(CLOSE_TYPE_STOP_LOSS);
			break;

	}

	OnHit();

}

////////////////////////////////////////////////////////////////////////////////

StopPrice::Params::Params(const Price &price)
	: m_price(price) {
	//....//
}

const Price & StopPrice::Params::GetPrice() const {
	return m_price;
}

StopPrice::StopPrice(
		const boost::shared_ptr<const Params> &params,
		Position &position)
	: StopLossOrder(position)
	, m_params(params) {
	//...//
}

StopPrice::~StopPrice() {
	//...//
}

const char * StopPrice::GetName() const {
	return "stop price";
}

bool StopPrice::Activate() {

	const auto &currentPrice = GetPosition().GetSecurity().GetLastPrice();
	if (GetPosition().IsLong()) {
		if (m_params->GetPrice() < currentPrice) {
			return false;
		}
	} else if (m_params->GetPrice() > currentPrice) {
		return false;
	}

	GetTradingLog().Write(
		"%1%\thit\tprice=%2$.8f%3%%4$.8f\tbid/ask=%5$.8f/%6$.8f\tpos=%7%",
		[&](TradingRecord &record) {
			record
				% GetName() // 1
				% currentPrice // 2
				% (GetPosition().IsLong() ? "<=" : ">=") // 3
				% m_params->GetPrice() // 4
				% GetPosition().GetSecurity().GetBidPriceValue() // 5
				% GetPosition().GetSecurity().GetAskPriceValue() // 6
				% GetPosition().GetId(); // 7
		});

	return true;

}

////////////////////////////////////////////////////////////////////////////////

StopLoss::Params::Params(const Volume &maxLossPerLot)
	: m_maxLossPerLot(maxLossPerLot) {
	//....//
}

const Volume & StopLoss::Params::GetMaxLossPerLot() const {
	return m_maxLossPerLot;
}

StopLoss::StopLoss(
		const boost::shared_ptr<const Params> &params,
		Position &position)
	: StopLossOrder(position)
	, m_params(params) {
	//...//
}

StopLoss::~StopLoss() {
	//...//
}

const char * StopLoss::GetName() const {
	return "stop loss";
}

bool StopLoss::Activate() {

	const Double maxLoss
		= GetPosition().GetOpenedQty() * -m_params->GetMaxLossPerLot();
	const auto &plannedPnl = GetPosition().GetPlannedPnl();

	if (maxLoss < plannedPnl) {
		return false;
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
				% m_params->GetMaxLossPerLot()
				% maxLoss
				% GetPosition().GetSecurity().GetBidPriceValue()
				% GetPosition().GetSecurity().GetAskPriceValue()
				% GetPosition().GetId();
		});

	return true;

}

////////////////////////////////////////////////////////////////////////////////

