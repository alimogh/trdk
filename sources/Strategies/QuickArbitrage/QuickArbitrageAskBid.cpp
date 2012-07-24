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

bool AskBid::IsValidSread() const {
	const DynamicSecurity &security = *GetSecurity();
	const auto spread = security.GetBidScaled() - security.GetAskScaled();
	const auto cmpVal = m_settings.isAbsoluteSpread
		?	m_settings.spread.absolute
		:	Security::Price(boost::math::round(security.GetBidScaled() * m_settings.spread.percents));
	return spread >= cmpVal;
}

bool AskBid::IsLongPosEnabled() const {
	if (m_settings.longPos.openMode == Settings::OPEN_MODE_NONE) {
		return false;
	}
	return IsValidSread();
}

bool AskBid::IsShortPosEnabled() const {
	if (m_settings.longPos.openMode == Settings::OPEN_MODE_NONE) {
		return false;
	}
	return IsValidSread();
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

		static const char * ConvertToStr(Settings::OpenMode mode) {
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

		static Settings::OrderType ConvertStrToOrderType(const std::string &str) {
			if (str == "IOC") {
				return Settings::ORDER_TYPE_IOC;
			} else if (str == "MKT") {
				return Settings::ORDER_TYPE_MKT;
			} else {
				throw IniFile::KeyFormatError("possible values: IOC, MKT");
			}
		}

		static const char * ConvertToStr(Settings::OrderType type) {
			switch (type) {
				case Settings::ORDER_TYPE_IOC:
					return "IOC";
				case  Settings::ORDER_TYPE_MKT:
					return "MKT";
				default:
					AssertFail("Unknown order type.");
					return "";
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
			settings.longPos.openMode = Settings::OPEN_MODE_NONE;
		} else if (mode == "bid") {
			settings.longPos.openMode = Settings::OPEN_MODE_BID;
		} else if (mode == "ask") {
			settings.longPos.openMode = Settings::OPEN_MODE_ASK;
		} else {
			throw IniFile::KeyFormatError("possible values: none, ask, bid");
		}
	}
	
	settings.openOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "open_order_type", false));
	settings.closeOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "close_order_type", false));

	{
		std::string spread = ini.ReadKey(section, "spread", false);
		settings.isAbsoluteSpread = !boost::ends_with(spread, "%");
		try {
			if (!settings.isAbsoluteSpread) {
				spread.pop_back();
				settings.spread.percents = boost::lexical_cast<double>(spread) / 100;
			} else {
				settings.spread.absolute = GetSecurity()->Scale(boost::lexical_cast<double>(spread));
			}
		} catch (const boost::bad_lexical_cast &ex) {
			Log::Error("Failed to parse \"spread\" key value (%2%): \"%1%\".", ex.what(), spread);
			throw IniFile::KeyFormatError("Failed to parse \"spread\" key");
		}
	}

	settings.takeProfit
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "take_profit"));
	settings.stopLoss
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "stop_loss"));
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

	settings.positionTimeSeconds
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_time_seconds"));

	SettingsReport settingsReport;
	AppendSettingsReport("open_shorts", Util::ConvertToStr(settings.shortPos.openMode), settingsReport);
	AppendSettingsReport("open_longs", Util::ConvertToStr(settings.longPos.openMode), settingsReport);
	AppendSettingsReport(
		"spread",
		settings.isAbsoluteSpread
			?	boost::lexical_cast<std::string>(GetSecurity()->Descale(settings.spread.absolute))
			:	(boost::lexical_cast<std::string>(settings.spread.percents * 100) + std::string("%")),
		settingsReport);
	AppendSettingsReport("take_profit", GetSecurity()->Descale(settings.takeProfit), settingsReport);
	AppendSettingsReport("stop_loss", GetSecurity()->Descale(settings.stopLoss), settingsReport);
	AppendSettingsReport("volume", GetSecurity()->Descale(settings.volume), settingsReport);
	AppendSettingsReport("position_time_seconds", settings.positionTimeSeconds, settingsReport);
	AppendSettingsReport("open_order_type", Util::ConvertToStr(settings.openOrderType), settingsReport);
	AppendSettingsReport("close_order_type", Util::ConvertToStr(settings.closeOrderType), settingsReport);
	ReportSettings(settingsReport);

	m_settings = settings;

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
	const bool isLoss
		= asIs
		|| (m_settings.closeOrderType == Settings::ORDER_TYPE_IOC 
			&& position.GetStopLoss() >= security.GetAskScaled());
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseLongPositionStopLossDo(position);
		} else {
			ReportTakeProfitDo(position);
			switch (m_settings.closeOrderType) {
				case Settings::ORDER_TYPE_IOC:
					security.SellOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
					position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
					break;
				case Settings::ORDER_TYPE_MKT:
					security.Sell(position.GetOpenedQty(), position);
					position.SetCloseType(Position::CLOSE_TYPE_NONE);
					break;
				default:
					AssertFail("Unknown close type.");
					break;
			}
			position.SetAlgoFlag(STATE_CLOSING);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseLongPositionStopLossDo(position);
	} else if (isLoss) {
		CloseLongPositionStopLossTry(position);
	} else if (position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT) {
		Assert(position.GetAlgoFlag() == STATE_CLOSING);
		Assert(position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT);
		position.ResetState();
		switch (m_settings.closeOrderType) {
			case Settings::ORDER_TYPE_IOC:
				position.SetTakeProfit(position.GetTakeProfit() - security.Scale(.01));
				ReportTakeProfitDo(position);
				security.SellOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
				break;
			case Settings::ORDER_TYPE_MKT:
				ReportTakeProfitDo(position);
				security.Sell(position.GetOpenedQty(), position);
				position.SetCloseType(Position::CLOSE_TYPE_NONE);
				break;
			default:
				AssertFail("Unknown close type.");
				break;
		}
	}
}

void AskBid::CloseShortPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	DynamicSecurity &security = *GetSecurity();
	bool isLoss
		= asIs
		|| (m_settings.closeOrderType == Settings::ORDER_TYPE_IOC 
			&& position.GetStopLoss() <= security.GetBidScaled());
	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			CloseShortPositionStopLossDo(position);
		} else {
			ReportTakeProfitDo(position);
			switch (m_settings.closeOrderType) {
				case Settings::ORDER_TYPE_IOC:
					security.BuyOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
					position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
					break;
				case Settings::ORDER_TYPE_MKT:
					security.Buy(position.GetOpenedQty(), position.GetTakeProfit(), position);
					position.SetCloseType(Position::CLOSE_TYPE_NONE);
					break;
				default:
					AssertFail("Unknown close type.");
					break;
			}
			position.SetAlgoFlag(STATE_CLOSING);
		}
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		CloseShortPositionStopLossDo(position);
	} else if (isLoss) {
		CloseShortPositionStopLossTry(position);
	} else if (position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT) {
		Assert(position.GetAlgoFlag() == STATE_CLOSING);
		Assert(position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT);
		position.ResetState();
		switch (m_settings.closeOrderType) {
			case Settings::ORDER_TYPE_IOC:
				position.SetTakeProfit(position.GetTakeProfit() + security.Scale(.01));
				ReportTakeProfitDo(position);
				security.BuyOrCancel(position.GetOpenedQty(), position.GetTakeProfit(), position);
				break;
			case Settings::ORDER_TYPE_MKT:
				ReportTakeProfitDo(position);
				security.Buy(position.GetOpenedQty(), position);
				position.SetCloseType(Position::CLOSE_TYPE_NONE);
				break;
			default:
				AssertFail("Unknown close type.");
				break;
		}
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
				&& position.GetOpenTime() + m_settings.positionTimeSeconds > boost::get_system_time()) {
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

void AskBid::DoOpenBuy(Position &position) {
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			GetSecurity()->BuyOrCancel(position.GetPlanedQty(), position.GetStartPrice(), position);
			break;
		case Settings::ORDER_TYPE_MKT:
			GetSecurity()->Buy(position.GetPlanedQty(), position);
			break;
		default:
			AssertFail("Unknown open order type.");
			break;
	}
}

void AskBid::DoOpenSell(Position &position) {
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			GetSecurity()->SellOrCancel(position.GetPlanedQty(), position.GetStartPrice(), position);
			break;
		case Settings::ORDER_TYPE_MKT:
			GetSecurity()->Sell(position.GetPlanedQty(), position);
			break;
		default:
			AssertFail("Unknown open order type.");
			break;
	}
}
