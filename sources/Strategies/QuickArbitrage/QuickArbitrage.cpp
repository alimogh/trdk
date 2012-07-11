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

namespace {

	enum PositionState {
		STATE_OPENING				= 1,
		STATE_CLOSING_TRY_STOP_LOSS	= 2,
		STATE_CLOSING				= 3
	};

	const char *const logTag = "quick-arbitrage";

	const std::string algoName = "Quick Arbitrage";

}

s::Algo::Algo(
			boost::shared_ptr<DynamicSecurity> security,
			const IniFile &ini,
			const std::string &section)
		: Base(security) {
	DoSettingsUpodate(ini, section);
}

s::Algo::~Algo() {
	//...//
}

void s::Algo::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpodate(ini, section);
}

void s::Algo::DoSettingsUpodate(const IniFile &ini, const std::string &section) {
	
	Settings settings = {};
	settings.shortPos.isEnabled = ini.ReadBoolKey(section, "open_shorts");
	if (settings.shortPos.isEnabled) {
		settings.shortPos.priceMod
			= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "short_open_price"));
	}
	settings.longPos.isEnabled = ini.ReadBoolKey(section, "open_longs");
	if (settings.longPos.isEnabled) {
		settings.longPos.priceMod
			= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "long_open_price"));
	}
	settings.takeProfit
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "take_profit"));
	settings.stopLoss
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "stop_loss"));
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

	m_settings = settings;

	Log::Info(
		"Settings: algo \"%1%\" for \"%2%\":"
			" open_shorts = %3%; short_open_price = %4%; open_longs = %5%; long_open_price = %6%;"
			" take_profit = %7%; stop_loss = %8%; volume = %9%;",
		algoName,
		GetSecurity()->GetFullSymbol(),
		m_settings.shortPos.isEnabled ? "yes" : "no",
		GetSecurity()->Descale(m_settings.shortPos.priceMod),
		m_settings.longPos.isEnabled ? "yes" : "no",
		GetSecurity()->Descale(m_settings.longPos.priceMod),
		GetSecurity()->Descale(m_settings.takeProfit),
		GetSecurity()->Descale(m_settings.stopLoss),
		GetSecurity()->Descale(m_settings.volume));

}

const std::string & s::Algo::GetName() const {
	return algoName;
}

std::auto_ptr<PositionReporter> s::Algo::CreatePositionReporter() const {
	std::auto_ptr<PositionReporter> result(new PositionReporterAlgo<s::Algo>(*this));
	return result;
}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> s::Algo::TryToOpenPositions() {
	boost::shared_ptr<PositionBandle> result(new PositionBandle);
	if (m_settings.longPos.isEnabled) {
		result->Get().push_back(OpenLongPosition());
	}
	if (m_settings.shortPos.isEnabled) {
		result->Get().push_back(OpenShortPosition());
	}
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenLongPosition() {
	
	DynamicSecurity &security = *GetSecurity();
	const auto ask = security.GetAskScaled();
	const auto bid = security.GetBidScaled();
	const auto price = ask - m_settings.longPos.priceMod;
	const auto takeProfit = price + m_settings.takeProfit;
	const auto stopLoss = price - m_settings.stopLoss;
	
	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_LONG,
			CalcQty(price, m_settings.volume),
			price,
			ask,
			bid,
			takeProfit,
			stopLoss,
			STATE_OPENING));
	security.BuyOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);

	return result;

}

boost::shared_ptr<Position> s::Algo::OpenShortPosition() {
	
	DynamicSecurity &security = *GetSecurity();
	const auto ask = security.GetAskScaled();
	const auto bid = security.GetBidScaled();
	const auto price = bid + m_settings.shortPos.priceMod;
	const auto takeProfit = price - m_settings.takeProfit;
	const auto stopLoss = price + m_settings.stopLoss;

	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_SHORT,
			CalcQty(price, m_settings.volume),
			price,
			ask,
			bid,
			takeProfit,
			stopLoss,
			STATE_OPENING));
	security.SellOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);
	
	return result;

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

void s::Algo::CloseLongPosition(Position &position) {
	Assert(position.GetType() == Position::TYPE_LONG);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = position.GetStopLoss() >= security.GetAskScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseLongPositionStopLossDo(position);
		} else {
			security.Sell(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseLongPositionStopLossDo(position);
	} else if (isLoss) {
		CloseLongPositionStopLossTry(position);
	}
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

void s::Algo::CloseShortPosition(Position &position) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = position.GetStopLoss() <= security.GetBidScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseShortPositionStopLossDo(position);
		} else {
			security.Buy(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseShortPositionStopLossDo(position);
	} else if (isLoss) {
		CloseShortPositionStopLossTry(position);
	}
}

void s::Algo::ClosePosition(Position &position) {
	
	Assert(
		position.GetAlgoFlag() == STATE_OPENING
		|| position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS
		|| position.GetAlgoFlag() == STATE_CLOSING);
	
	if (!position.IsOpened()) {
		Assert(!position.IsClosed());
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
			CloseLongPosition(position);
			break;
		case Position::TYPE_SHORT:
			CloseShortPosition(position);
			break;
		default:
			AssertFail("Unknown position type.");
			break;
	}

	Assert(position.GetAlgoFlag() != STATE_OPENING);

}

void s::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p);
	}
}

void s::Algo::ReportDecision(const Position &position) const {
	Log::Trading(
		logTag,
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6% take-profit=%7% stop-loss=%8%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetDecisionAks()),
		position.GetSecurity().Descale(position.GetDecisionBid()),
		position.GetSecurity().Descale(position.GetStartPrice()),
		position.GetPlanedQty(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().Descale(position.GetStopLoss()));
}

void s::Algo::ReportStopLossTry(const Position &position) const {
	Log::Trading(
		logTag,
		"%1% %2% stop-loss-try cur-ask-bid=%3%/%4% stop-loss=%5% qty=%6%->%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetStopLoss()),
		position.GetOpenedQty(),
		position.GetOpenedQty() - position.GetClosedQty());
}

void s::Algo::ReportStopLossDo(const Position &position) const {
	Log::Trading(
		logTag,
		"%1% %2% stop-loss-do cur-ask-bid=%3%/%4% stop-loss=%5% qty=%6%->%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetStopLoss()),
		position.GetOpenedQty(),
		position.GetOpenedQty() - position.GetClosedQty());
}

