/**************************************************************************
 *   Created: 2012/07/10 01:25:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "QuickArbitrageOld.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporterAlgo.hpp"

using namespace Strategies::QuickArbitrage;

namespace {

	const char *const logTag = "quick-arbitrage-old";

	const std::string algoName = "Quick Arbitrage Old";

}

Old::Old(
			boost::shared_ptr<DynamicSecurity> security,
			const IniFile &ini,
			const std::string &section)
		: Base(security, logTag) {
	DoSettingsUpdate(ini, section);
}

Old::~Old() {
	//...//
}

std::auto_ptr<PositionReporter> Old::CreatePositionReporter() const {
	std::auto_ptr<PositionReporter> result(new PositionReporterAlgo<decltype(*this)>(*this));
	return result;
}

bool Old::IsLongPosEnabled() const {
	return m_settings.longPos.isEnabled;
}
bool Old::IsShortPosEnabled() const {
	return m_settings.shortPos.isEnabled;
}

Security::Price Old::GetLongPriceMod() const {
	return m_settings.longPos.priceMod;
}

Security::Price Old::GetShortPriceMod() const {
	return m_settings.shortPos.priceMod;
}

Security::Price Old::GetTakeProfit() const {
	return m_settings.takeProfit;
}

Security::Price Old::GetStopLoss() const {
	return m_settings.stopLoss;
}

Security::Price Old::GetVolume() const {
	return m_settings.volume;
}

void Old::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpdate(ini, section);
}

void Old::DoSettingsUpdate(const IniFile &ini, const std::string &section) {
	
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

const std::string & Old::GetName() const {
	return algoName;
}

void Old::CloseLongPosition(Position &position, bool asIs) {
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

void Old::CloseShortPosition(Position &position, bool asIs) {
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

void Old::ClosePosition(Position &position, bool asIs) {
	
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
