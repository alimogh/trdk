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
#include "Core/AlgoPositionState.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/PositionBundle.hpp"

namespace s = Strategies::Level2MarketArbitrage;
namespace pt = boost::posix_time;

namespace {
	
	const std::string algoName = "Level II Market Arbitrage";

	enum PositionState {
		STATE_OPENING				= 1,
		STATE_CLOSING_TRY_STOP_LOSS	= 2,
		STATE_CLOSING				= 3
	};

}

//////////////////////////////////////////////////////////////////////////

class s::Algo::State : public  AlgoPositionState {

public:

	State() {
		//...//
	}

	virtual ~State() {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

s::Algo::Algo(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFile &ini,
			const std::string &section)
		: Base(tag, security) {
	{
		const Settings settings = {};
		m_settings = settings;
	}
	DoSettingsUpdate(ini, section);
}

s::Algo::~Algo() {
	//...//
}

const std::string & s::Algo::GetName() const {
	return algoName;
}

void s::Algo::SubscribeToMarketData(
			const LiveMarketDataSource &iqFeed,
			const LiveMarketDataSource &interactiveBrokers) {
	iqFeed.SubscribeToMarketDataLevel1(GetSecurity());
	switch (m_settings.level2DataSource) {
		case Settings::MARKET_DATA_SOURCE_IQFEED:
			iqFeed.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			interactiveBrokers.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		default:
			AssertFail("Unknown market data source.");
	}
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

	settings.entryAskBidDifferencePercent
		= ini.ReadTypedKey<double>(section, "ask_bid_difference_percent_entry") / 100;
	settings.stopLossAskBidDifferencePercent
		= ini.ReadTypedKey<double>(section, "ask_bid_difference_percent_stop_loss") / 100;
	if (settings.entryAskBidDifferencePercent <= settings.stopLossAskBidDifferencePercent) {
		throw IniFile::KeyFormatError("Stop loss doesn't make sense");
	}

	settings.takeProfit = ini.ReadAbsoluteOrPercentsPriceKey(
		section,
		"take_profit",
		GetSecurity()->GetScale());
	settings.volume
		= GetSecurity()->Scale(ini.ReadTypedKey<double>(section, "volume"));

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
		static const char * ConvertToStr(Settings::OrderType type) {
			switch (type) {
				case Settings::ORDER_TYPE_IOC:
					return "IOC";
				case Settings::ORDER_TYPE_MKT:
					return "MKT";
				default:
					AssertFail("Unknown order type.");
					return "";
			}
		}
		static const char * ConvertToStr(Settings::MarketDataSource marketDataSource) {
			switch (marketDataSource) {
				case Settings::MARKET_DATA_SOURCE_IQFEED:
					return "IQFeed";
				case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
					return "Interactive Brokers";
				case Settings::MARKET_DATA_SOURCE_NOT_SET:
					AssertFail("Market data source not set.");
					return "";
				default:
					AssertFail("Unknown market data Level II source.");
					return "";
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
		static Settings::MarketDataSource ConvertStrToMarketDataSource(const std::string &str) {
			if (	boost::iequals(str, "IQFeed")
					|| boost::iequals(str, "IQ")
					|| boost::iequals(str, "IQFeed.net")) {
				return Settings::MARKET_DATA_SOURCE_IQFEED;
			} else if (
					boost::iequals(str, "Interactive Brokers")
					|| boost::iequals(str, "InteractiveBrokers")
					|| boost::iequals(str, "IB")) {
				return Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS;
			} else {
				throw IniFile::KeyFormatError("possible values: IOC, MKT");
			}
		}
	};
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

	settings.positionTimeSeconds
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_time_seconds"));

	settings.openOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "open_order_type", false));
	settings.closeOrderType
		= Util::ConvertStrToOrderType(ini.ReadKey(section, "close_order_type", false));

	settings.level2DataSource = m_settings.level2DataSource
		?	m_settings.level2DataSource
		:	Util::ConvertStrToMarketDataSource(ini.ReadKey(section, "level2_data_source", false));

	SettingsReport report;
	AppendPercentSettingsReport(
		"ask_bid_difference_percent_entry",
		settings.entryAskBidDifferencePercent * 100,
		report);
	AppendPercentSettingsReport(
		"ask_bid_difference_percent_stop_loss",
		settings.stopLossAskBidDifferencePercent * 100,
		report);
	AppendSettingsReport("take_profit", settings.takeProfit, report);
	AppendSettingsReport("open_mode", Util::ConvertToStr(settings.openMode), report);
	AppendSettingsReport("position_time_seconds", settings.positionTimeSeconds, report);
	AppendSettingsReport("open_order_type", Util::ConvertToStr(settings.openOrderType), report);
	AppendSettingsReport("close_order_type", Util::ConvertToStr(settings.closeOrderType), report);
	AppendSettingsReport("level2_data_source", Util::ConvertToStr(settings.level2DataSource), report);
	AppendSettingsReport("volume", GetSecurity()->Descale(settings.volume), report);
	ReportSettings(report);

	m_settings = settings;
	UpdateCallbacks();

}

void s::Algo::UpdateCallbacks() {
	switch (m_settings.level2DataSource) {
		case Settings::MARKET_DATA_SOURCE_IQFEED:
			m_askSizeGetter = boost::bind(&Security::GetAskSizeIqFeed, GetSecurity());
			m_bidSizeGetter = boost::bind(&Security::GetBidSizeIqFeed, GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			m_askSizeGetter = boost::bind(&Security::GetAskSizeIb, GetSecurity());
			m_bidSizeGetter = boost::bind(&Security::GetBidSizeIb, GetSecurity());
			break;
		default:
			AssertFail("Unknown market data source.");
	}

}

Security::Qty s::Algo::GetShortStopLoss(
			Security::Qty askSize,
			Security::Qty bidSize)
		const {
	switch (m_settings.openMode) {
		case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
			return Security::Qty(boost::math::round(bidSize * m_settings.stopLossAskBidDifferencePercent));
			break;
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return Security::Qty(boost::math::round(askSize * m_settings.stopLossAskBidDifferencePercent));
			break;
		default:
			AssertFail("Unknown open mode");
			throw Exception("Unknown open mode");
	}
}

Security::Qty s::Algo::GetLongStopLoss(
			Security::Qty askSize,
			Security::Qty bidSize)
		const {
	switch (m_settings.openMode) {
		case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
			return Security::Qty(boost::math::round(askSize * m_settings.stopLossAskBidDifferencePercent));
			break;
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return Security::Qty(boost::math::round(bidSize * m_settings.stopLossAskBidDifferencePercent));
			break;
		default:
			AssertFail("Unknown open mode");
			throw Exception("Unknown open mode");
	}
}

boost::shared_ptr<Position> s::Algo::OpenShortPostion(
			Security::Qty askSize,
			Security::Qty bidSize) {
	const auto price = GetSecurity()->GetAskScaled();
	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_SHORT,
			CalcQty(price, m_settings.volume),
			price,
			askSize,
			bidSize,
			price - m_settings.takeProfit.Get(price),
			GetShortStopLoss(askSize, bidSize),
			STATE_OPENING,
			shared_from_this(),
			boost::shared_ptr<AlgoPositionState>(new State)));
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			GetSecurity()->SellOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);
			break;
		case Settings::ORDER_TYPE_MKT:
			GetSecurity()->Sell(result->GetPlanedQty(), *result);
			break;
		default:
			AssertFail("Unknown order type.");
			break;
	}
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenLongPostion(
			Security::Qty askSize,
			Security::Qty bidSize) {
	const auto price = GetSecurity()->GetBidScaled();
	boost::shared_ptr<Position> result(
		new Position(
			GetSecurity(),
			Position::TYPE_LONG,
			CalcQty(price, m_settings.volume),
			price,
			askSize,
			bidSize,
			price + m_settings.takeProfit.Get(price),
			GetLongStopLoss(askSize, bidSize),
			STATE_OPENING,
			shared_from_this(),
			boost::shared_ptr<AlgoPositionState>(new State)));
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			GetSecurity()->BuyOrCancel(result->GetPlanedQty(), result->GetStartPrice(), *result);
			break;
		case Settings::ORDER_TYPE_MKT:
			GetSecurity()->Buy(result->GetPlanedQty(), *result);
			break;
		default:
			AssertFail("Unknown order type.");
			break;
	}
	return result;
}

boost::shared_ptr<PositionBandle> s::Algo::TryToOpenPositions() {
	
	boost::shared_ptr<PositionBandle> result;

	const auto askSize = m_askSizeGetter();
	const auto bidSize = m_bidSizeGetter();
	if (	!askSize
			|| !bidSize
			|| m_settings.openMode == Settings::OPEN_MODE_NONE) {
		return result;
	}

	if (askSize > bidSize * m_settings.entryAskBidDifferencePercent) {
		// we expect prices to drop (since many sellers have shown up temporarily)
		result.reset(new PositionBandle);
		switch (m_settings.openMode) {
			case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
				result->Get().push_back(OpenShortPostion(askSize, bidSize));
				break;
			case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
				result->Get().push_back(OpenLongPostion(askSize, bidSize));
				break;
			default:
				AssertFail("Unknown open mode");
				break;
		}
	} else if (bidSize > askSize * m_settings.entryAskBidDifferencePercent) {
		// we expect prices to increase as many buyers have shown up
		result.reset(new PositionBandle);
		switch (m_settings.openMode) {
			case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
				result->Get().push_back(OpenLongPostion(askSize, bidSize));
				break;
			case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
				result->Get().push_back(OpenShortPostion(askSize, bidSize));
				break;
			default:
				AssertFail("Unknown open mode");
				break;
		}
	}

	GetSecurity()->ReportLevel2Snapshot();

	return result;

}

template<
	typename TakeProfitCheckMethod,
	typename StopLossCheckMethod,
	typename GetTakeProfitMethod>
void s::Algo::CloseAbstractPosition(
			Position &position,
			bool asIs,
			TakeProfitCheckMethod takeProfitCheckMethod,
			StopLossCheckMethod stopLossCheckMethod,
			GetTakeProfitMethod getTakeProfitMethod,
			Security::Qty (s::Algo::*getStopLossMethod)(Security::Qty, Security::Qty) const,
			void (Security::*iocCloseMethod)(Security::Qty, Security::Price, Position &),
			void (Security::*mktCloseMethod)(Security::Qty, Position &),
			void (s::Algo::*closeLongPositionStopLossDoMethod)(Position &),
			void (s::Algo::*closeLongPositionStopLossTryMethod)(Position &)) {
	
	Security &security = *GetSecurity();
	
	const bool isLoss = asIs || stopLossCheckMethod();

	if (position.GetAlgoFlag() == STATE_OPENING) {
		if (isLoss) {
			(this->*closeLongPositionStopLossDoMethod)(position);
		} else {
			position.SetTakeProfit(getTakeProfitMethod());
			position.SetStopLoss((this->*getStopLossMethod)(m_askSizeGetter(), m_bidSizeGetter()));
			ReportCloseTry(position);
			switch (m_settings.closeOrderType) {
				case Settings::ORDER_TYPE_IOC:
					(security.*iocCloseMethod)(
						position.GetOpenedQty(),
						position.GetTakeProfit(),
						position);
					position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
					break;
				case Settings::ORDER_TYPE_MKT:
					if (takeProfitCheckMethod()) {
						(security.*mktCloseMethod)(position.GetOpenedQty(), position);
						position.SetCloseType(Position::CLOSE_TYPE_TAKE_PROFIT);
					}
					break;
				default:
					AssertFail("Unknown order type.");
					break;
			}
		}
		position.SetAlgoFlag(STATE_CLOSING);
	} else if (position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS) {
		(this->*closeLongPositionStopLossDoMethod)(position);
	} else if (isLoss) {
		(this->*closeLongPositionStopLossTryMethod)(position);
	} else {
		Assert(position.IsCloseCanceled());
		Assert(position.GetCloseType() == Position::CLOSE_TYPE_TAKE_PROFIT);
		position.ResetState();
		switch (m_settings.closeOrderType) {
			case Settings::ORDER_TYPE_IOC:
				ReportCloseTry(position);
				(security.*iocCloseMethod)(
					position.GetOpenedQty() - position.GetClosedQty(),
					position.GetTakeProfit(),
					position);
				break;
			case Settings::ORDER_TYPE_MKT:
				AssertFail("Settings::ORDER_TYPE_MKT");
				break;
			default:
				AssertFail("Unknown order type.");
				break;
		}
	}
}

void s::Algo::CloseLongPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_LONG);
	CloseAbstractPosition(
		position,
		asIs,
		[&]() -> bool {
			return position.GetTakeProfit() <= GetSecurity()->GetAskScaled();
		},
		[&]() -> bool {
			switch (m_settings.openMode) {
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
					return m_bidSizeGetter() > m_askSizeGetter() * m_settings.stopLossAskBidDifferencePercent;
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
					return m_askSizeGetter() > m_bidSizeGetter() * m_settings.stopLossAskBidDifferencePercent;
				default:
					AssertFail("Unknown open mode");
					return true;
			}
		},
		[&]() -> Security::Price {
			const auto price = position.GetOpenPrice();
			return price + m_settings.takeProfit.Get(price);
		},
		&Algo::GetLongStopLoss,
		&Security::SellOrCancel,
		&Security::Sell,
		&Algo::CloseLongPositionStopLossDo,
		&Algo::CloseLongPositionStopLossTry);
}

void s::Algo::CloseShortPosition(Position &position, bool asIs) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	CloseAbstractPosition(
		position,
		asIs,
		[&]() -> bool {
			return position.GetTakeProfit() >= GetSecurity()->GetBidScaled();
		},
		[&]() -> bool {
			switch (m_settings.openMode) {
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
					return m_askSizeGetter() > m_bidSizeGetter() * m_settings.entryAskBidDifferencePercent;
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
					return m_bidSizeGetter() > m_askSizeGetter() * m_settings.entryAskBidDifferencePercent;
				default:
					AssertFail("Unknown open mode");
					return true;
			}
		},
		[&]() -> Security::Price {
			const auto price = position.GetOpenPrice();
			return price - m_settings.takeProfit.Get(price);
		},
		&Algo::GetShortStopLoss,
		&Security::BuyOrCancel,
		&Security::Buy,
		&Algo::CloseShortPositionStopLossDo,
		&Algo::CloseShortPositionStopLossTry);
}

void s::Algo::ClosePosition(Position &position, bool asIs) {

	Assert(
		position.GetAlgoFlag() == STATE_OPENING
		|| position.GetAlgoFlag() == STATE_CLOSING_TRY_STOP_LOSS
		|| position.GetAlgoFlag() == STATE_CLOSING);
	
	if (!position.IsOpened()) {
		Assert(!position.IsClosed());
		if (asIs) {
			if (position.GetOpenOrderId()) {
				GetSecurity()->CancelOrder(position.GetOpenOrderId());
			}
		}
		return;
	} else if (position.IsClosed()) {
		return;
	} else if (position.GetAlgoFlag() == STATE_CLOSING && !position.IsCloseCanceled()) {
		return;
	} else if (!asIs && position.GetAlgoFlag() == STATE_OPENING) {
		Assert(!position.GetOpenTime().is_not_a_date_time());
		const auto currentTime = GetCurrentTime();
		if (position.GetOpenTime() + m_settings.positionTimeSeconds > currentTime) {
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

void s::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p, false);
	}
	GetSecurity()->ReportLevel2Snapshot();
}

void s::Algo::ClosePositionsAsIs(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p, true);
	}
	GetSecurity()->ReportLevel2Snapshot();
}

void s::Algo::ReportDecision(const Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try size-ask-bid=%3%/%4% limit-used=%5% qty=%6% take-profit=%7% stop-loss=%8%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		Security::Qty(position.GetDecisionAks()),
		Security::Qty(position.GetDecisionBid()),
		position.GetSecurity().Descale(position.GetStartPrice()),
		position.GetPlanedQty(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().Descale(position.GetStopLoss()));
}

void s::Algo::ReportCloseTry(const Position &position) {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% close-try limit-price=%3% open-price=%4% opened-qty=%5%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().Descale(position.GetTakeProfit()),
		position.GetSecurity().Descale(position.GetOpenPrice()),
		position.GetOpenedQty() - position.GetClosedQty());

}

void s::Algo::ClosePositionStopLossTry(Position &position) {
	position.SetCloseType(Position::CLOSE_TYPE_STOP_LOSS);
	ReportStopLossTry(position);
	GetSecurity()->CancelOrder(position.GetOpenOrderId());
	position.SetAlgoFlag(STATE_CLOSING_TRY_STOP_LOSS);
}

void s::Algo::CloseLongPositionStopLossDo(Position &position) {
	Assert(position.GetType() == Position::TYPE_LONG);
	position.SetCloseType(Position::CLOSE_TYPE_STOP_LOSS);
	ReportStopLossDo(position);
	GetSecurity()->Sell(position.GetOpenedQty() - position.GetClosedQty(), position);
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
	position.SetAlgoFlag(STATE_CLOSING);
}

void s::Algo::CloseShortPositionStopLossTry(Position &position) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	ClosePositionStopLossTry(position);
}

