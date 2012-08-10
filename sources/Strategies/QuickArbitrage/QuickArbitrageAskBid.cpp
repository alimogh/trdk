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

//////////////////////////////////////////////////////////////////////////

namespace {

	const std::string algoName = "Quick Arbitrage AskBid";

}

//////////////////////////////////////////////////////////////////////////

AskBid::AskBid(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFile &ini,
			const std::string &section)
		: Base(tag, security) {
	DoSettingsUpdate(ini, section);
}

AskBid::~AskBid() {
	//...//
}

std::auto_ptr<PositionReporter> AskBid::CreatePositionReporter() const {
	class Reporter : public PositionReporterAlgo<AskBid> {
	public:
		typedef PositionReporterAlgo<AskBid> Base;
	public:
		virtual ~Reporter() {
			//...//
		}
	protected:
		virtual void PrintHead(std::ostream &out) const {
			Base::PrintHead(out);
			out
				<< ",ask/bid at entry"
				<< ",ask/bid at exit"
				<< ",t/p price"
				<< ",s/l price";
		}
		virtual void PrintLine(const Position &position, std::ostream &out) const {
			Base::PrintLine(position, out);
			const State &state = position.GetAlgoState<State>();
			const Security &sec = position.GetSecurity();
			out
				// "ask/bid at entry"
				<< ',' << sec.Descale(state.entry.ask) << '/' << sec.Descale(state.entry.bid)
				// "ask/bid at exit"
				<< ',' << sec.Descale(state.exit.ask) << '/' << sec.Descale(state.exit.bid)
				// t/p price
				<< ',' << sec.Descale(state.takeProfit)
				// s/l price
				<< ',' <<  sec.Descale(state.stopLoss);
		}
	};

	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return result;

}

bool AskBid::IsValidSread(Security::Price valGt, Security::Price valLs) const {
	const auto spread = valGt - valLs;
	if (spread < 0) {
		return false;
	}
	return spread >= m_settings.spread.Get(valGt);
}

bool AskBid::IsLongPosEnabled() const {
	if (!m_settings.isLongsEnabled) {
		return false;
	}
	switch (m_settings.openMode) {
		case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
			return IsValidSread(GetSecurity()->GetBidScaled(), GetSecurity()->GetAskScaled());
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return IsValidSread(GetSecurity()->GetAskScaled(), GetSecurity()->GetBidScaled());
		default:
			AssertFail("Unknown open mode.");
		case Settings::OPEN_MODE_NONE:
			return false;
	}
}

bool AskBid::IsShortPosEnabled() const {
	if (!m_settings.isShortsEnabled) {
		return false;
	}
	switch (m_settings.openMode) {
		case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
			return IsValidSread(GetSecurity()->GetAskScaled(), GetSecurity()->GetBidScaled());
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return IsValidSread(GetSecurity()->GetBidScaled(), GetSecurity()->GetAskScaled());
		default:
			AssertFail("Unknown open mode.");
		case Settings::OPEN_MODE_NONE:
			return false;
	}
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
				case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
					return "short_if_ask>bid";
				case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
					return "short_if_bid>ask";
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
		const std::string mode = ini.ReadKey(section, "open_mode", false);
		if (mode == "none") {
			settings.openMode = Settings::OPEN_MODE_NONE;
		} else if (mode == "short_if_ask>bid") {
			settings.openMode = Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID;
		} else if (mode == "short_if_bid>ask") {
			settings.openMode = Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK;
		} else {
			throw IniFile::KeyFormatError("open_mode possible values: none, short_if_ask>bid, short_if_bid>ask");
		}
	}

	settings.isLongsEnabled = ini.ReadBoolKey(section, "longs_enabled");
	settings.isShortsEnabled = ini.ReadBoolKey(section, "shorts_enabled");

	settings.openOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "open_order_type", false));
	settings.closeOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "close_order_type", false));

	settings.spread = ini.ReadAbsoluteOrPercentsPriceKey(
		section,
		"spread",
		GetSecurity()->GetScale());

	if (settings.closeOrderType == Settings::ORDER_TYPE_IOC) {
		settings.takeProfit
			= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "take_profit"));
		settings.stopLoss
			= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "stop_loss"));
	}
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

	settings.positionTimeSeconds
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_time_seconds"));

	SettingsReport settingsReport;
	AppendSettingsReport("open_mode", Util::ConvertToStr(settings.openMode), settingsReport);
	AppendSettingsReport("longs_enabled", settings.isLongsEnabled, settingsReport);
	AppendSettingsReport("shorts_enabled", settings.isShortsEnabled, settingsReport);
	AppendSettingsReport("spread", settings.spread, settingsReport);
	if (settings.closeOrderType == Settings::ORDER_TYPE_IOC) {
		AppendSettingsReport("take_profit", GetSecurity()->Descale(settings.takeProfit), settingsReport);
		AppendSettingsReport("stop_loss", GetSecurity()->Descale(settings.stopLoss), settingsReport);
	}
	AppendSettingsReport("volume", GetSecurity()->Descale(settings.volume), settingsReport);
	AppendSettingsReport("position_time_seconds", settings.positionTimeSeconds, settingsReport);
	AppendSettingsReport("open_order_type", Util::ConvertToStr(settings.openOrderType), settingsReport);
	AppendSettingsReport("close_order_type", Util::ConvertToStr(settings.closeOrderType), settingsReport);
	ReportSettings(settingsReport);

	m_settings = settings;

}

Security::Price AskBid::ChooseLongOpenPrice(
			Security::Price ask,
			Security::Price /*bid*/)
		const {
	Assert(m_settings.isLongsEnabled);
	/* 07/25/2012:
		[1:42:24] Eugene V. Palchukovsky: when we use IOC (now useing MKT) and open long - we use current bid for price, righ?
		[1:42:45] Torsten Jacobi: bid/ask depening on what trade direction we go
		[1:42:58] Torsten Jacobi: long ask, short bid
	 */
	return ask;
}

Security::Price AskBid::ChooseShortOpenPrice(
			Security::Price /*ask*/,
			Security::Price bid)
		const {
	Assert(m_settings.isShortsEnabled);
	/* 07/25/2012:
		[1:42:24] Eugene V. Palchukovsky: when we use IOC (now useing MKT) and open long - we use current bid for price, righ?
		[1:42:45] Torsten Jacobi: bid/ask depening on what trade direction we go
		[1:42:58] Torsten Jacobi: long ask, short bid
	 */
	return bid;
}

const std::string & AskBid::GetName() const {
	return algoName;
}

void AskBid::ClosePosition(Position &position) {

	if (!position.IsOpened() || position.IsClosed() || position.IsCanceled()) {
		return;
	} else if (	!position.GetAlgoState<State>().isStarted
			&& position.GetOpenTime() + m_settings.positionTimeSeconds > GetCurrentTime()) {
		return;
	}

	DoClosePosition(position);
	position.GetAlgoState<State>().isStarted = true;

}

void AskBid::DoClosePosition(Position &position) {

	State &state = position.GetAlgoState<State>();
	Security &security = *GetSecurity();

	Security::Price price = 0;
	bool isLoss = false;
	switch (position.GetType()) {
		case Position::TYPE_LONG:
			price = security.GetAskScaled();
			isLoss
				= (m_settings.closeOrderType == Settings::ORDER_TYPE_IOC 
					&& state.stopLoss >= price);
			break;
		case Position::TYPE_SHORT:
			price = security.GetBidScaled();
			isLoss
				= m_settings.closeOrderType == Settings::ORDER_TYPE_IOC 
					&& state.stopLoss <= price;
			break;
		default:
			AssertFail("Unknown position type");
			return;
	}

	if (position.HasActiveOrders() && !isLoss) {
		Assert(!position.HasActiveOpenOrders());
		Assert(state.isStarted);
		return;
	} 

	position.SetCloseStartPrice(price);
	state.exit.ask = security.GetAskScaled();
	state.exit.bid = security.GetBidScaled();

	if (isLoss) {
		ReportStopLoss(position);
		position.CancelAtMarketPrice(Position::CLOSE_TYPE_STOP_LOSS);
		return;
	}

	if (state.isStarted) {
		switch (position.GetType()) {
			case Position::TYPE_LONG:
				state.takeProfit -= security.Scale(.01);
				break;
			case Position::TYPE_SHORT:
				state.takeProfit += security.Scale(.01);
				break;
			default:
				AssertFail("Unknown position type");
				return;
		}
	}

	ReportTakeProfitDo(position);
	switch (m_settings.closeOrderType) {
		case Settings::ORDER_TYPE_IOC:
			position.CloseOrCancel(Position::CLOSE_TYPE_TAKE_PROFIT, state.takeProfit);
			break;
		case Settings::ORDER_TYPE_MKT:
			position.CloseAtMarketPrice(Position::CLOSE_TYPE_TAKE_PROFIT);
			break;
		default:
			AssertFail("Unknown close type.");
			return;
	}

	state.isStarted = true;

}

void AskBid::ReportTakeProfitDo(const Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% take-profit-do limit-price=%3% cur-ask-bid=%4%/%5% stop-loss=%6% qty=%7%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetAlgoState<State>().takeProfit),
		position.GetSecurity().GetAsk(),
		position.GetSecurity().GetBid(),
		position.GetSecurity().Descale(position.GetAlgoState<State>().stopLoss),
		position.GetOpenedQty());
}

void AskBid::DoOpen(Position &position) {
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			position.OpenOrCancel(position.GetOpenStartPrice());
			break;
		case Settings::ORDER_TYPE_MKT:
			position.OpenAtMarketPrice();
			break;
		default:
			AssertFail("Unknown open order type.");
			break;
	}
}
