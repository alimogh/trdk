/**************************************************************************
 *   Created: 2012/07/12 20:58:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Prec.hpp"
#include "QuickArbitrage.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporterAlgo.hpp"
#include "Core/MarketDataSource.hpp"

namespace s = Strategies::QuickArbitrage;

s::Algo::Algo(
			const std::string &tag,
			boost::shared_ptr<Security> security)
		: Base(tag, security) {
	//...//
}

s::Algo::~Algo() {
	//...//
}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> s::Algo::TryToOpenPositions() {
	boost::shared_ptr<PositionBandle> result(new PositionBandle);
	if (IsLongPosEnabled()) {
		result->Get().push_back(OpenLongPosition());
	}
	if (IsShortPosEnabled()) {
		result->Get().push_back(OpenShortPosition());
	}
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenLongPosition() {
	Security &security = *GetSecurity();
	const auto ask = security.GetAskScaled();
	const auto bid = security.GetBidScaled();
	const auto price = ChooseLongOpenPrice(ask, bid) - GetLongPriceMod();
	boost::shared_ptr<Position> result(
		new LongPosition(
			GetSecurity(),
			CalcQty(price, GetVolume()),
			price,
			shared_from_this(),
			boost::shared_ptr<AlgoPositionState>(
				new State(ask, bid, price + GetTakeProfit(), price - GetStopLoss()))));
	DoOpen(*result);
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenShortPosition() {
	Security &security = *GetSecurity();
	const auto ask = security.GetAskScaled();
	const auto bid = security.GetBidScaled();
	const auto price = ChooseShortOpenPrice(ask, bid) + GetShortPriceMod();
	boost::shared_ptr<Position> result(
		new ShortPosition(
			GetSecurity(),
			CalcQty(price, GetVolume()),
			price,
			shared_from_this(),
			boost::shared_ptr<AlgoPositionState>(
				new State(ask, bid, price - GetTakeProfit(), price + GetStopLoss()))));
	DoOpen(*result);
	return result;
}

void s::Algo::ClosePositionStopLossTry(Position &position) {
	ReportStopLossTry(position);
	position.CancelAllOrders();
	position.GetAlgoState<State>().state = STATE_CLOSING_TRY_STOP_LOSS;
}

void s::Algo::ClosePositionStopLossDo(Position &position) {
	ReportStopLossDo(position);
	position.CloseAtMarketPrice();
	position.SetCloseType(Position::CLOSE_TYPE_STOP_LOSS);
	position.GetAlgoState<State>().state = STATE_CLOSING;
}

void s::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p, false);
	}
}

void s::Algo::ClosePositionsAsIs(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p, true);
	}
}

void s::Algo::ReportDecision(const Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6% take-profit=%7% stop-loss=%8%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetAlgoState<State>().entry.ask),
		position.GetSecurity().Descale(position.GetAlgoState<State>().entry.bid),
		position.GetSecurity().Descale(position.GetOpenStartPrice()),
		position.GetPlanedQty(),
		position.GetSecurity().Descale(position.GetAlgoState<State>().takeProfit),
		position.GetSecurity().Descale(position.GetAlgoState<State>().stopLoss));
}

void s::Algo::SubscribeToMarketData(
			const LiveMarketDataSource &iqFeed,
			const LiveMarketDataSource &/*interactiveBrokers*/) {
	iqFeed.SubscribeToMarketDataLevel1(GetSecurity());
}
