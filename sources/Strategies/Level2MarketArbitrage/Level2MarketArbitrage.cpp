/**************************************************************************
 *   Created: 2012/07/16 01:41:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Level2MarketArbitrage.hpp"
#include "Core/PositionReporterAlgo.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/PositionBundle.hpp"

namespace s = Strategies::Level2MarketArbitrage;

namespace {
	
	const char *const logTag = "level-2-arbitrage";
	const std::string algoName = "Level II Market Arbitrage";

	enum PositionState {
		STATE_OPENING				= 1,
		STATE_CLOSING_TRY_STOP_LOSS	= 2,
		STATE_CLOSING				= 3
	};

}

s::Algo::Algo(
			boost::shared_ptr<DynamicSecurity> security,
			const IniFile &ini,
			const std::string &section)
		: Base(security, logTag) {
	DoSettingsUpdate(ini, section);
}

s::Algo::~Algo() {
	//...//
}

const std::string & s::Algo::GetName() const {
	return algoName;
}

void s::Algo::SubscribeToMarketData(MarketDataSource &marketDataSource) {
	marketDataSource.SubscribeToMarketDataLevel1(GetSecurity());
	marketDataSource.SubscribeToMarketDataLevel2(GetSecurity());
}

void s::Algo::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpdate(ini, section);
}

std::auto_ptr<PositionReporter> s::Algo::CreatePositionReporter() const {
	std::auto_ptr<PositionReporter> result(new PositionReporterAlgo<decltype(*this)>(*this));
	return result;
}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

void s::Algo::DoSettingsUpdate(const IniFile &ini, const std::string &section) {
	
	Settings settings = {};

	settings.askBidDifferencePercent
		= ini.ReadTypedKey<double>(section, "ask_bid_difference_percent") / 100;
	settings.takeProfitPercent
		= ini.ReadTypedKey<double>(section, "take_profit_percent") / 100;
	settings.stopLossPercent
		= ini.ReadTypedKey<double>(section, "stop_loss_percent") / 100;
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

	m_settings = settings;
	Log::Info(
		"Settings: algo \"%1%\" for \"%2%\":"
			" ask_bid_difference_percent = %3%%%; take_profit_percent = %4%%%; stop_loss_percent = %5%%%; volume = %6%",
		algoName,
		GetSecurity()->GetFullSymbol(),
		settings.askBidDifferencePercent * 100,
		settings.takeProfitPercent * 100,
		settings.stopLossPercent * 100,
		settings.volume);

}

boost::shared_ptr<Position> s::Algo::OpenShortPostion(
			DynamicSecurity::Qty askSize,
			DynamicSecurity::Qty bidSize) {
	const auto price = GetSecurity()->GetAskScaled();
	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_SHORT,
			CalcQty(price, m_settings.volume),
			price,
			askSize,
			bidSize,
			price - Security::Price(boost::math::round(price * m_settings.takeProfitPercent)),
			price + Security::Price(boost::math::round(price * m_settings.stopLossPercent)),
			STATE_OPENING));
	GetSecurity()->SellOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenLongPostion(
			DynamicSecurity::Qty askSize,
			DynamicSecurity::Qty bidSize) {
	const auto price = GetSecurity()->GetBidScaled();
	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_LONG,
			CalcQty(price, m_settings.volume),
			price,
			askSize,
			bidSize,
			price + Security::Price(boost::math::round(price * m_settings.takeProfitPercent)),
			price - Security::Price(boost::math::round(price * m_settings.stopLossPercent)),
			STATE_OPENING));
	GetSecurity()->BuyOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);
	return result;
}

boost::shared_ptr<PositionBandle> s::Algo::TryToOpenPositions() {
	
	boost::shared_ptr<PositionBandle> result;
	
	const auto askSize = GetSecurity()->GetAskSize();
	const auto bidSize = GetSecurity()->GetBidSize();
	if (!askSize || !bidSize) {
		return result;
	}

	if (askSize > bidSize * m_settings.askBidDifferencePercent) {
		// we expect prices to drop (since many sellers have shown up temporarily)
		result.reset(new PositionBandle);
		result->Get().push_back(OpenShortPostion(askSize, bidSize));
	} else if (bidSize > askSize * m_settings.askBidDifferencePercent) {
		// we expect prices to increase as many buyers have shown up
		result.reset(new PositionBandle);
		result->Get().push_back(OpenLongPostion(askSize, bidSize));
	}

	return result;

}

void s::Algo::CloseLongPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_LONG);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = asIs || position.GetStopLoss() >= security.GetAskScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseLongPositionStopLossDo(position);
		} else {
			security.Sell(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
			position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseLongPositionStopLossDo(position);
	} else if (isLoss) {
		CloseLongPositionStopLossTry(position);
	}
}

void s::Algo::CloseShortPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = asIs || position.GetStopLoss() <= security.GetBidScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseShortPositionStopLossDo(position);
		} else {
			security.Buy(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
			position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseShortPositionStopLossDo(position);
	} else if (isLoss) {
		CloseShortPositionStopLossTry(position);
	}
}

void s::Algo::ClosePosition(Position &position, bool asIs) {

	Assert(
		position.GetAlgoFlag() == STATE_OPENING
		|| position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS
		|| position.GetAlgoFlag() == STATE_CLOSING);
	
	if (!position.IsOpened()) {
		Assert(!position.IsClosed());
		if (asIs) {
			GetSecurity()->CancelAllOrders();
		}
		return;
	} else if (position.IsClosed()) {
		return;
	} else if (
			position.GetAlgoFlag() == STATE_CLOSING
			&& position.GetCloseType() == Position::CLOSE_TYPE_STOP_LOSS) {
		return;
	}

	switch (position.GetType()) {
		case Position::TYPE_LONG:
			CloseLongPosition(position, asIs);
			break;
		case Position::TYPE_SHORT:
			CloseShortPosition(position, asIs);
			break;
		default:
			AssertFail("Unknown position type.");
			break;
	}

	Assert(position.GetAlgoFlag() != STATE_OPENING);

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
		logTag,
		"%1% %2% open-try size-ask-bid=%3%/%4% limit-used=%5% qty=%6% take-profit=%7% stop-loss=%8%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		DynamicSecurity::Qty(position.GetDecisionAks()),
		DynamicSecurity::Qty(position.GetDecisionBid()),
		position.GetSecurity().Descale(position.GetStartPrice()),
		position.GetPlanedQty(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().Descale(position.GetStopLoss()));
}

void s::Algo::ClosePositionStopLossTry(Position &position) {
	ReportStopLossTry(position);
	GetSecurity()->CancelAllOrders();
	position.SetAlgoFlag(STATE_CLOSING_TRY_STOP_LOSS);
}

void s::Algo::CloseLongPositionStopLossDo(Position &position) {
	Assert(position.GetType() == Position::TYPE_LONG);
	ReportStopLossDo(position);
	GetSecurity()->Sell(position.GetOpenedQty() - position.GetClosedQty(), position);
	position.SetCloseType(Position::CLOSE_TYPE_STOP_LOSS);
	position.SetAlgoFlag(STATE_CLOSING);
}

void s::Algo::CloseLongPositionStopLossTry(Position &position) {
	Assert(position.GetType() == Position::TYPE_LONG);
	ClosePositionStopLossTry(position);
}

void s::Algo::CloseShortPositionStopLossDo(Position &position) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	ReportStopLossDo(position);
	GetSecurity()->Buy(position.GetOpenedQty() - position.GetClosedQty(), position);
	position.SetCloseType(Position::CLOSE_TYPE_STOP_LOSS);
	position.SetAlgoFlag(STATE_CLOSING);
}

void s::Algo::CloseShortPositionStopLossTry(Position &position) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	ClosePositionStopLossTry(position);
}

