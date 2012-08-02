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

	const std::string algoName = "Quick Arbitrage Old";

}

Old::Old(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFile &ini,
			const std::string &section)
		: Base(tag, security) {
	DoSettingsUpdate(ini, section);
}

Old::~Old() {
	//...//
}

std::auto_ptr<PositionReporter> Old::CreatePositionReporter() const {
	typedef PositionReporterAlgo<decltype(*this)> Reporter;
	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return result;
}

bool Old::IsLongPosEnabled() const {
	return m_settings.longPos.openMode != Settings::OPEN_MODE_NONE;
}
bool Old::IsShortPosEnabled() const {
	return m_settings.shortPos.openMode != Settings::OPEN_MODE_NONE;
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

	struct Util {	
		static const char * ConvertOpenModeToStr(Settings::OpenMode mode) {
			switch (mode) {
				default:
					AssertFail("Unknown open mode.");
				case Settings::OPEN_MODE_NONE:
					return "none";
				case Settings::OPEN_MODE_BID:
					return "bid";
				case Settings::OPEN_MODE_ASK:
					return "ask";
			}
		}
	};

	Settings settings = {};

	{
		const std::string mode = ini.ReadKey(section, "open_shorts", false);
		if (mode == "none") {
			settings.shortPos.openMode = Settings::OPEN_MODE_NONE;
		} else if (mode == "bid") {
			settings.shortPos.openMode = Settings::OPEN_MODE_BID;
		} else if (mode == "ask") {
			settings.shortPos.openMode = Settings::OPEN_MODE_ASK;
		} else {
			throw IniFile::KeyFormatError("open_shorts possible values: none, ask, bid");
		}
		if (settings.shortPos.openMode != Settings::OPEN_MODE_NONE) {
			settings.shortPos.priceMod
				= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "short_open_price"));
		}
	}
	{
		const std::string mode = ini.ReadKey(section, "open_longs", false);
		if (mode == "none") {
			settings.longPos.openMode = Settings::OPEN_MODE_NONE;
		} else if (mode == "bid") {
			settings.longPos.openMode = Settings::OPEN_MODE_BID;
		} else if (mode == "ask") {
			settings.longPos.openMode = Settings::OPEN_MODE_ASK;
		} else {
			throw IniFile::KeyFormatError("possible values: none, ask, bid");
		}
		if (settings.longPos.openMode != Settings::OPEN_MODE_NONE) {
			settings.longPos.priceMod
				= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "long_open_price"));
		}
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
		Util::ConvertOpenModeToStr(m_settings.shortPos.openMode),
		GetSecurity()->Descale(m_settings.shortPos.priceMod),
		Util::ConvertOpenModeToStr(m_settings.longPos.openMode),
		GetSecurity()->Descale(m_settings.longPos.priceMod),
		GetSecurity()->Descale(m_settings.takeProfit),
		GetSecurity()->Descale(m_settings.stopLoss),
		GetSecurity()->Descale(m_settings.volume));

}

Security::Price Old::ChooseLongOpenPrice(
			Security::Price ask,
			Security::Price bid)
		const {
	switch (m_settings.longPos.openMode) {
		case Settings::OPEN_MODE_BID:
			return bid;
		case Settings::OPEN_MODE_ASK:
			return ask;
		default:
			AssertFail("Failed to get position open price.");
			throw Exception("Failed to get position open price");
	}
}

Security::Price Old::ChooseShortOpenPrice(
			Security::Price ask,
			Security::Price bid)
		const {
	switch (m_settings.shortPos.openMode) {
		case Settings::OPEN_MODE_BID:
			return bid;
		case Settings::OPEN_MODE_ASK:
			return ask;
		default:
			AssertFail("Failed to get position open price.");
			throw Exception("Failed to get position open price");
	}
}

const std::string & Old::GetName() const {
	return algoName;
}

void Old::CloseLongPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_LONG);
	Security &security = *GetSecurity();
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
	Security &security = *GetSecurity();
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

void Old::DoOpenBuy(Position &position) {
	GetSecurity()->BuyOrCancel(position.GetPlanedQty(), position.GetOpenStartPrice(), position);
}

void Old::DoOpenSell(Position &position) {
	GetSecurity()->SellOrCancel(position.GetPlanedQty(), position.GetOpenStartPrice(), position);
}
