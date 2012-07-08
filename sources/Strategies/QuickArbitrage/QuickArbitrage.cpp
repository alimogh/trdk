/**************************************************************************
 *   Created: 2012/07/10 01:25:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "QuickArbitrage.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporterAlgo.hpp"

namespace s = Strategies::QuickArbitrage;

s::Algo::Algo(boost::shared_ptr<DynamicSecurity> security)
		: Base(security) {
	//...//
}

s::Algo::~Algo() {
	//...//
}

const std::string & s::Algo::GetName() const {
	static const std::string name = "Quick Arbitrage";
	return name;
}

std::auto_ptr<PositionReporter> s::Algo::CreatePositionReporter() const {
	std::auto_ptr<PositionReporter> result(new PositionReporterAlgo<s::Algo>(*this));
	return result;
}

Security::Price s::Algo::CalcShortPrice() const {
	return GetSecurity()->GetBidScaled() + GetSecurity()->Scale(0.04);
}

Security::Price s::Algo::CalcLongTakeProfit(Security::Price openPrice) const {
	return openPrice + GetSecurity()->Scale(0.04);
}

Security::Price s::Algo::CalcLongStopLoss(Security::Price openPrice) const {
	return openPrice - GetSecurity()->Scale(0.04);
}

Security::Price s::Algo::CalcLongPrice() const {
	return GetSecurity()->GetAskScaled() - GetSecurity()->Scale(0.04);
}

Security::Price s::Algo::CalcShortTakeProfit(Security::Price openPrice) const {
	return openPrice - GetSecurity()->Scale(0.04);
}

Security::Price s::Algo::CalcShortStopLoss(Security::Price openPrice) const {
	return openPrice + GetSecurity()->Scale(0.04);
}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> s::Algo::OpenPositions() {
	boost::shared_ptr<PositionBandle> result(new PositionBandle);
	DynamicSecurity &security = *GetSecurity();
	const auto ask = security.GetAskScaled();
	const auto bid = security.GetBidScaled();
	{
		const auto price = CalcLongPrice();
		boost::shared_ptr<Position> position(
			new Position(
				GetSecurity(),
				Position::TYPE_LONG,
				CalcQty(price),
				price,
				ask,
				bid,
				CalcLongTakeProfit(price),
				CalcLongStopLoss(price)));
		security.BuyOrCancel(
			position->GetPlanedQty(),
			position->GetStartPrice(),
			bind(&Position::UpdateOpening, position, _1, _2, _3, _4, _5, _6));
		result->Get().push_back(position);
	}
	{
		const auto price = CalcShortPrice();
		boost::shared_ptr<Position> position(
			new Position(
				GetSecurity(),
				Position::TYPE_SHORT,
				CalcQty(price),
				price,
				ask,
				bid,
				CalcShortTakeProfit(price),
				CalcShortStopLoss(price)));
		security.SellOrCancel(
			position->GetPlanedQty(),
			position->GetStartPrice(),
			bind(&Position::UpdateOpening, position, _1, _2, _3, _4, _5, _6));
		result->Get().push_back(position);
	}
	return result;
}

void s::Algo::ClosePositions(PositionBandle &positions) {
	foreach (auto pos, positions.Get()) {
		if (!pos->IsOpened()) {
			continue;
		}
		switch (pos->GetType()) {
			case Position::TYPE_LONG:
				break;
			case Position::TYPE_SHORT:
				break;
			default:
				AssertFail("Unknown position type.");
				break;
		}
	}
}

void s::Algo::ReportDecision(const Position &position) const {
	Log::Trading(
		"quick-arbitrage",
		"%1% %2% cur-ask-bid=%3%/%4% limit-used=%5% number-of-shares=%6% take-profit-price=%7% stop-loss-price=%8%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetDecisionAks()),
		position.GetSecurity().Descale(position.GetDecisionBid()),
		position.GetSecurity().Descale(position.GetStartPrice()),
		position.GetPlanedQty(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().Descale(position.GetStopLoss()));
}
