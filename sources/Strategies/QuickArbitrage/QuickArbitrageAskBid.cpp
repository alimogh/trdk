/**************************************************************************
 *   Created: 2012/07/10 01:25:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "QuickArbitrageAskBid.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporterAlgo.hpp"

using namespace Strategies::QuickArbitrage;
namespace pt = boost::posix_time;

namespace {

	const char *const logTag = "quick-arbitrage-ab";

	const std::string algoName = "Quick Arbitrage AskBid";

}

AskBid::AskBid(
			boost::shared_ptr<DynamicSecurity> security,
			const IniFile &ini,
			const std::string &section)
		: Base(security, logTag) {
	DoSettingsUpdate(ini, section);
}

AskBid::~AskBid() {
	//...//
}

std::auto_ptr<PositionReporter> AskBid::CreatePositionReporter() const {
	std::auto_ptr<PositionReporter> result(new PositionReporterAlgo<decltype(*this)>(*this));
	return result;
}

bool AskBid::IsLongPosEnabled() const {
	if (m_settings.longPos.openMode == Settings::OPEN_MODE_NONE) {
		return false;
	}
	const DynamicSecurity &security = *GetSecurity();
	return security.GetBidScaled() - security.GetAskScaled() >= m_settings.askBidDifference;
	
}
bool AskBid::IsShortPosEnabled() const {
	if (m_settings.longPos.openMode == Settings::OPEN_MODE_NONE) {
		return false;
	}
	const DynamicSecurity &security = *GetSecurity();
	return security.GetBidScaled() - security.GetAskScaled() >= m_settings.askBidDifference;
}

Security::Price AskBid::GetLongPriceMod() const {
	return 0;
}

Security::Price AskBid::GetShortPriceMod() const {
	return 0;
}

Security::Price AskBid::GetTakeProfit() const {
	return m_settings.takeProfit;
}

Security::Price AskBid::GetStopLoss() const {
	return m_settings.stopLoss;
}

Security::Price AskBid::GetVolume() const {
	return m_settings.volume;
}

void AskBid::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpdate(ini, section);
}

void AskBid::DoSettingsUpdate(const IniFile &ini, const std::string &section) {

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
	}
	{
		const std::string mode = ini.ReadKey(section, "open_longs", false);
		if (mode == "none") {
			settings.shortPos.openMode = Settings::OPEN_MODE_NONE;
		} else if (mode == "bid") {
			settings.shortPos.openMode = Settings::OPEN_MODE_BID;
		} else if (mode == "ask") {
			settings.shortPos.openMode = Settings::OPEN_MODE_ASK;
		} else {
			throw IniFile::KeyFormatError("possible values: none, ask, bid");
		}
	}

	settings.askBidDifference
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "ask_bid_difference"));

	settings.takeProfit
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "take_profit"));
	settings.stopLoss
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "stop_loss"));
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

	settings.positionTime
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_time_secs"));

	m_settings = settings;

	Log::Info(
		"Settings: algo \"%1%\" for \"%2%\":"
			" open_shorts = %3%; open_longs = %4%; ask_bid_difference = %5%;"
			" take_profit = %6%; stop_loss = %7%; volume = %8%; position_time = %9%",
		algoName,
		GetSecurity()->GetFullSymbol(),
		Util::ConvertOpenModeToStr(m_settings.shortPos.openMode),
		Util::ConvertOpenModeToStr(m_settings.longPos.openMode),
		GetSecurity()->Descale(m_settings.askBidDifference),
		GetSecurity()->Descale(m_settings.takeProfit),
		GetSecurity()->Descale(m_settings.stopLoss),
		GetSecurity()->Descale(m_settings.volume),
		m_settings.positionTime);

}

Security::Price AskBid::ChooseLongOpenPrice(
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

Security::Price AskBid::ChooseShortOpenPrice(
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

const std::string & AskBid::GetName() const {
	return algoName;
}

void AskBid::CloseLongPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_LONG);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = asIs || position.GetStopLoss() >= security.GetAskScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseLongPositionStopLossDo(position);
		} else {
			ReportTakeProfitDo(position);
			security.SellOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
			position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseLongPositionStopLossDo(position);
	} else if (isLoss) {
		CloseLongPositionStopLossTry(position);
	} else if (position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT) {
		Assert(position.GetAlgoFlag() == STATE_CLOSING);
		ReportTakeProfitDo(position);
		position.ResetState();
		security.SellOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
	}
}

void AskBid::CloseShortPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	DynamicSecurity &security = *GetSecurity();
	const bool isLoss = asIs || position.GetStopLoss() <= security.GetBidScaled();
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseShortPositionStopLossDo(position);
		} else {
			ReportTakeProfitDo(position);
			security.BuyOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
			position.SetAlgoFlag(STATE_CLOSING);
			position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseShortPositionStopLossDo(position);
	} else if (isLoss) {
		CloseShortPositionStopLossTry(position);
	} else if (position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT) {
		Assert(position.GetAlgoFlag() == STATE_CLOSING);
		ReportTakeProfitDo(position);
		position.ResetState();
		security.BuyOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
	}
}

void AskBid::ClosePosition(Position &position, bool asIs) {
	
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
	} else if (position.IsOpenError()) {
		return;
	} else if (position.IsClosed()) {
		return;
	} else if (
			position.GetAlgoFlag() == STATE_CLOSING
			&& (position.GetCloseType() == Position::CLOSE_TYPE_STOP_LOSS
				|| (!position.IsCloseCanceled() && position.IsOpened()))) {
		return;
	} else if (!asIs && position.GetAlgoFlag() == STATE_OPENING) {
		Assert(!position.GetOpenTime().is_not_a_date_time());
		if (	!position.GetOpenTime().is_not_a_date_time()
				&& position.GetOpenTime() + m_settings.positionTime > boost::get_system_time()) {
			return;
		}
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

void AskBid::ReportTakeProfitDo(const Position &position) const {
	Log::Trading(
		GetLogTag().c_str(),
		"%1% %2% take-profit-do limit-price=%3% cur-ask-bid=%4%/%5% stop-loss=%6% qty=%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetStopLoss()),
		position.GetOpenedQty());
}

