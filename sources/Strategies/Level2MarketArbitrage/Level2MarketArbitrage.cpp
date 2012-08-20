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

}

//////////////////////////////////////////////////////////////////////////

class s::Algo::State : public  AlgoPositionState {

public:

	bool isStarted;

	pt::ptime lastStateReport;

	Security::Price takeProfit;
	Security::Price stopLoss;

	struct AsksBids {
		
		double ratioIni;
		double ratio;
		Security::Price ask;
		Security::Price bid;
		Security::Qty askSize;
		Security::Qty bidSize;

		AsksBids()
				: ratioIni(0),
				ratio(0),
				ask(0),
				bid(0),
				askSize(0),
				bidSize(0) {
			//...//
		}

		explicit AsksBids(
					double ratioIniIn,
					double ratioIn,
					Security::Price askIn,
					Security::Price bidIn,
					Security::Qty askSizeIn,
					Security::Qty bidSizeIn)
				: ratioIni(ratioIniIn),
				ratio(ratioIn),
				ask(askIn),
				bid(bidIn),
				askSize(askSizeIn),
				bidSize(bidSizeIn) {
			//...//
		}

	};

	const AsksBids entry;
	AsksBids exit;

public:

	explicit State(
				double entryRatioIni,
				double entryRatio,
				Security::Price entryAsk,
				Security::Price entryBid,
				Security::Qty entryAskSize,
				Security::Qty entryBidSize)
			: isStarted(false),
			takeProfit(0),
			stopLoss(0),
			entry(entryRatioIni, entryRatio, entryAsk, entryBid, entryAskSize, entryBidSize) {
		//...//
	}

	~State() {
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

	class Reporter : public PositionReporterAlgo<s::Algo> {
	public:
		typedef PositionReporterAlgo<s::Algo> Base;
	public:
		virtual ~Reporter() {
			//...//
		}
	protected:
		virtual void PrintHead(std::ostream &out) const {
			Base::PrintHead(out);
			out
				<< ",entry ratio (ini)"
				<< ",ratio at entry"
				<< ",ask/bid sizes at entry"
				<< ",ask/bid at entry"
				<< ",t/p price"
				<< ",s/l ratio (ini)"
				<< ",ratio at exit"
				<< ",ask/bid sizes at exit"
				<< ",ask/bid at exit";
		}
		virtual void PrintLine(const Position &position, std::ostream &out) const {
			
			Base::PrintLine(position, out);
			
			const State &state = position.GetAlgoState<State>();
			
			out
				
				// entry ratio (ini)
				<< ',' <<  state.entry.ratioIni

				// ratio at entry
				<< ',' << state.entry.ratio
				
				// ask/bid sizes at entry
				<< ',' << state.entry.askSize << '/' << state.entry.bidSize

				// ask/bid at entry
				<< ',' << position.GetSecurity().Descale(state.entry.ask)
					<< '/' << position.GetSecurity().Descale(state.entry.bid)

				// t/p price
				<< ',' << position.GetSecurity().Descale(state.takeProfit)
				
				// s/l ratio (ini)
				<< ',' <<  state.exit.ratioIni
				
				// ratio at exit
				<< ',' << state.exit.ratio
				
				// ask/bid sizes at exit
				<< ',' << state.exit.askSize << '/' << state.exit.bidSize

				// ask/bid at exit
				<< ',' << position.GetSecurity().Descale(state.exit.ask)
					<< '/' << position.GetSecurity().Descale(state.exit.bid);

		}
	};

	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return result;

}

void s::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

void s::Algo::DoSettingsUpdate(const IniFile &ini, const std::string &section) {
	
	Settings settings = {};

	settings.entryAskBidDifference
		= ini.ReadTypedKey<double>(section, "ask_bid_difference_percent_entry") / 100;
	settings.stopLossAskBidDifference
		= ini.ReadTypedKey<double>(section, "ask_bid_difference_percent_stop_loss") / 100;
	if (settings.entryAskBidDifference <= settings.stopLossAskBidDifference) {
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
				throw IniFile::KeyFormatError("possible values: Interactive Brokers, IQFeed");
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

	settings.positionMinTime
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_min_time_seconds"));
	settings.positionMaxTime
		= pt::seconds(ini.ReadTypedKey<long>(section, "position_max_time_seconds"));

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
		settings.entryAskBidDifference * 100,
		report);
	AppendPercentSettingsReport(
		"ask_bid_difference_percent_stop_loss",
		settings.stopLossAskBidDifference * 100,
		report);
	AppendSettingsReport("take_profit", settings.takeProfit, report);
	AppendSettingsReport("open_mode", Util::ConvertToStr(settings.openMode), report);
	AppendSettingsReport("position_min_time_seconds", settings.positionMinTime, report);
	AppendSettingsReport("position_max_time_seconds", settings.positionMaxTime, report);
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
			m_askSizeGetter = boost::bind(&Security::GetLevel2AskSizeIqFeed, GetSecurity());
			m_bidSizeGetter = boost::bind(&Security::GetLevel2BidSizeIqFeed, GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			m_askSizeGetter = boost::bind(&Security::GetLevel2AskSizeIb, GetSecurity());
			m_bidSizeGetter = boost::bind(&Security::GetLevel2BidSizeIb, GetSecurity());
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
			return Security::Qty(boost::math::round(bidSize * m_settings.stopLossAskBidDifference));
			break;
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return Security::Qty(boost::math::round(askSize * m_settings.stopLossAskBidDifference));
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
			return Security::Qty(boost::math::round(askSize * m_settings.stopLossAskBidDifference));
			break;
		case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
			return Security::Qty(boost::math::round(bidSize * m_settings.stopLossAskBidDifference));
			break;
		default:
			AssertFail("Unknown open mode");
			throw Exception("Unknown open mode");
	}
}

template<typename PositionT>
boost::shared_ptr<Position> s::Algo::OpenPostion(
			Security::Price startPrice,
			Security::Qty askSize,
			Security::Qty bidSize,
			double ratio) {
	boost::shared_ptr<Position> result(
		new PositionT(
			GetSecurity(),
			CalcQty(startPrice, m_settings.volume),
			startPrice,
			shared_from_this(),
			boost::shared_ptr<AlgoPositionState>(
				new State(
					m_settings.entryAskBidDifference,
					ratio,
					GetSecurity()->GetAskPriceScaled(),
					GetSecurity()->GetBidPriceScaled(),
					askSize,
					bidSize))));
	switch (m_settings.openOrderType) {
		case Settings::ORDER_TYPE_IOC:
			result->OpenOrCancel(result->GetOpenStartPrice());
			break;
		case Settings::ORDER_TYPE_MKT:
			result->OpenAtMarketPrice();
			break;
		default:
			AssertFail("Unknown order type.");
			break;
	}
	return result;
}

boost::shared_ptr<Position> s::Algo::OpenShortPostion(
			Security::Qty askSize,
			Security::Qty bidSize,
			double ratio) {
	return OpenPostion<ShortPosition>(
		GetSecurity()->GetAskPriceScaled(),
		askSize,
		bidSize,
		ratio);
}

boost::shared_ptr<Position> s::Algo::OpenLongPostion(
			Security::Qty askSize,
			Security::Qty bidSize,
			double ratio) {
	return OpenPostion<LongPosition>(
		GetSecurity()->GetBidPriceScaled(),
		askSize,
		bidSize,
		ratio);
}

boost::shared_ptr<PositionBandle> s::Algo::TryToOpenPositions() {
	
	boost::shared_ptr<PositionBandle> result;

	if (m_settings.openMode == Settings::OPEN_MODE_NONE) {
		return result;
	}

	const auto askSize = m_askSizeGetter();
	const auto bidSize = m_bidSizeGetter();
	if (!askSize || !bidSize) {
		ReportNoDecision(askSize, bidSize);
		return result;
	}

	if (double(askSize) >= double(bidSize) * m_settings.entryAskBidDifference) {
		// we expect prices to drop (since many sellers have shown up temporarily)
		const double ratio = double(askSize) / double(bidSize);
		result.reset(new PositionBandle);
		switch (m_settings.openMode) {
			case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
				result->Get().push_back(OpenShortPostion(askSize, bidSize, ratio));
				break;
			case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
				result->Get().push_back(OpenLongPostion(askSize, bidSize, ratio));
				break;
			default:
				AssertFail("Unknown open mode");
				break;
		}
	} else if (double(bidSize) >= double(askSize) * m_settings.entryAskBidDifference) {
		// we expect prices to increase as many buyers have shown up
		const double ratio = double(bidSize) / double(askSize);
		result.reset(new PositionBandle);
		switch (m_settings.openMode) {
			case Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
				result->Get().push_back(OpenLongPostion(askSize, bidSize, ratio));
				break;
			case Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
				result->Get().push_back(OpenShortPostion(askSize, bidSize, ratio));
				break;
			default:
				AssertFail("Unknown open mode");
				break;
		}
	} else {
		ReportNoDecision(askSize, bidSize);
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
			TakeProfitCheckMethod takeProfitCheckMethod,
			StopLossCheckMethod stopLossCheckMethod,
			GetTakeProfitMethod getTakeProfitMethod,
			Security::Qty (s::Algo::*getStopLossMethod)(Security::Qty, Security::Qty) const) {
	
	Security &security = *GetSecurity();
	State &state = position.GetAlgoState<State>();

	State::AsksBids exitAsksBids;
	state.exit.ask = security.GetAskPriceScaled();
	state.exit.bid = security.GetBidPriceScaled();
	state.exit.askSize = m_askSizeGetter();
	state.exit.bidSize = m_bidSizeGetter();
	state.stopLoss = (this->*getStopLossMethod)(state.exit.askSize, state.exit.bidSize);
	state.exit.ratioIni = m_settings.stopLossAskBidDifference;

	const bool isLoss = stopLossCheckMethod();
	const bool isTimeOver
		= !isLoss && position.GetOpenTime() + m_settings.positionMaxTime <= GetCurrentTime();

	if (position.HasActiveOrders() && !isLoss && !isTimeOver) {
		Assert(!position.HasActiveOpenOrders());
		Assert(state.isStarted);
		ReportStillOpened(position);
		return;
	}

	if (!state.isStarted) {
		Assert(!state.takeProfit);
		state.takeProfit = getTakeProfitMethod();
	}

	if (position.HasActiveOrders() || !takeProfitCheckMethod()) {
		if (isLoss || isTimeOver) {
			if (isLoss) {
				ReportClosing(position, false);
				position.CancelAtMarketPrice(Position::CLOSE_TYPE_STOP_LOSS);
				return;
			} else if (isTimeOver) {
				ReportClosingByTime(position);
				position.CancelAtMarketPrice(Position::CLOSE_TYPE_TIMEOUT);
				return;
			}
			AssertFail("isLoss || isTimeOver");
		} else {
			ReportStillOpened(position);
		}
		return;
	}
	
	ReportClosing(position, true);
	security.ReportLevel2Snapshot(true);

	switch (m_settings.closeOrderType) {
		case Settings::ORDER_TYPE_IOC:
			position.CloseOrCancel(Position::CLOSE_TYPE_TAKE_PROFIT, state.takeProfit);
			break;
		case Settings::ORDER_TYPE_MKT:
			position.CloseAtMarketPrice(Position::CLOSE_TYPE_TAKE_PROFIT);
			break;
		default:
			AssertFail("Unknown order type.");
			break;
	}

}

void s::Algo::CloseLongPosition(Position &position) {
	Assert(position.GetType() == Position::TYPE_LONG);
	// If we have a long position we should look at bid to determine our t/p goal, if short the ask position.
	position.SetCloseStartPrice(GetSecurity()->GetBidPriceScaled());
	CloseAbstractPosition(
		position,
		[&]() -> bool {
			return position.GetAlgoState<State>().takeProfit <= position.GetCloseStartPrice();
		},
		[&]() -> bool {
			State &state = position.GetAlgoState<State>();
			switch (m_settings.openMode) {
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
					state.exit.ratio = double(state.exit.bidSize) / double(state.exit.askSize);
					return double(state.exit.bidSize) <= double(state.exit.askSize) * state.exit.ratioIni;
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
					state.exit.ratio = double(state.exit.askSize) / double(state.exit.bidSize);
					return double(state.exit.askSize) <= double(state.exit.bidSize) * state.exit.ratioIni;
				default:
					AssertFail("Unknown open mode");
					return true;
			}
		},
		[&]() -> Security::Price {
			const auto price = position.GetOpenPrice();
			return price + m_settings.takeProfit.Get(price);
		},
		&Algo::GetLongStopLoss);
}

void s::Algo::CloseShortPosition(Position &position) {
	Assert(position.GetType() == Position::TYPE_SHORT);
	// If we have a long position we should look at bid to determine our t/p goal, if short the ask position.
	position.SetCloseStartPrice(GetSecurity()->GetAskPriceScaled());
	CloseAbstractPosition(
		position,
		[&]() -> bool {
			return position.GetAlgoState<State>().takeProfit >= position.GetCloseStartPrice();
		},
		[&]() -> bool {
			State &state = position.GetAlgoState<State>();
			switch (m_settings.openMode) {
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_ASK_MORE_BID:
					state.exit.ratio = double(state.exit.askSize) / double(state.exit.bidSize);
					return double(state.exit.askSize) <= double(state.exit.bidSize) * state.exit.ratioIni;
				case s::Algo::Settings::OPEN_MODE_SHORT_IF_BID_MORE_ASK:
					state.exit.ratio = double(state.exit.bidSize) / double(state.exit.askSize);
					return double(state.exit.bidSize) <= double(state.exit.askSize) * state.exit.ratioIni;
				default:
					AssertFail("Unknown open mode");
					return true;
			}
		},
		[&]() -> Security::Price {
			const auto price = position.GetOpenPrice();
			return price - m_settings.takeProfit.Get(price);
		},
		&Algo::GetShortStopLoss);
}

void s::Algo::ClosePosition(Position &position) {

	if (!position.IsOpened() || position.IsClosed() || position.IsCanceled()) {
		return;
	} else if (
			!position.GetAlgoState<State>().isStarted
			&& position.GetOpenTime() + m_settings.positionMinTime > GetCurrentTime()) {
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

	position.GetAlgoState<State>().isStarted = true;

}

void s::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		ClosePosition(*p);
	}
	GetSecurity()->ReportLevel2Snapshot();
}

void s::Algo::ReportDecision(const Position &position) const {
	const Security &sec = position.GetSecurity();
	const State &state = position.GetAlgoState<State>();
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try asks/bids=%3%/%4% ratio-ini/ratio=%5%/%6%",
		sec.GetSymbol(),
		position.GetTypeStr(),
		state.entry.askSize,
		state.entry.bidSize,
		state.entry.ratioIni,
		state.entry.ratio);
}

void s::Algo::ReportStillOpened(Position &position) {
	State &state = position.GetAlgoState<State>();
	if (	!state.takeProfit
			|| (!state.lastStateReport.is_not_a_date_time()
				&& state.lastStateReport + pt::seconds(10) > GetCurrentTime())) {
		return;
	}
	const Security &sec = position.GetSecurity();
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% still-opened open-order-id=%3% asks/bids=%4%/%5% s/l-ratio-ini/ratio=%6%/%7% t/p=%8% ask/bid=%9%/%10%",
		sec.GetSymbol(),
		position.GetTypeStr(),
		position.GetOpenOrderId(),
		state.exit.askSize,
		state.exit.bidSize,
		state.exit.ratioIni,
		state.exit.ratio,
		sec.Descale(state.takeProfit),
		sec.GetAskPrice(),
		sec.GetBidPrice());
	state.lastStateReport = GetCurrentTime();
}

void s::Algo::ReportNoDecision(Security::Qty askSize, Security::Qty bidSize) {
	if (	!m_lastStateReport.is_not_a_date_time()
			&& m_lastStateReport + pt::seconds(10) > GetCurrentTime()) {
		return;
	}
	const Security &sec = *GetSecurity();
	Log::Trading(
		GetTag().c_str(),
		"%1% no-decision asks/bids=%2%/%3% ratio-ini/ratio-1/ration-2=%4%/%5%/%6% ask/bid=%7%/%8%",
		sec.GetSymbol(),
		askSize,
		bidSize,
		m_settings.entryAskBidDifference,
		bidSize ? double(askSize) / double(bidSize) : 0,
		askSize ? double(bidSize) / double(askSize) : 0,
		sec.GetAskPrice(),
		sec.GetBidPrice());
	m_lastStateReport = GetCurrentTime();
}

void s::Algo::ReportClosing(const Position &position, bool isTakeProfit) {
	const Security &sec = position.GetSecurity();
	const State &state = position.GetAlgoState<State>();
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% %3% asks/bids=%4%/%5% s/l-ratio-ini/ratio=%6%/%7% ask/bid=%8%/%9% t/p=%10%",
		sec.GetSymbol(),
		position.GetTypeStr(),
		isTakeProfit ? "take-profit" : "stop-loss",
		state.exit.askSize,
		state.exit.bidSize,
		state.exit.ratioIni,
		state.exit.ratio,
		sec.GetAskPrice(),
		sec.GetBidPrice(),
		sec.Descale(state.takeProfit));
}

void s::Algo::ReportClosingByTime(const Position &position) {
	const Security &sec = position.GetSecurity();
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% close-by-time opened=%3% open-order-id=%4%",
		sec.GetSymbol(),
		position.GetTypeStr(),
		position.GetOpenTime().time_of_day(),
		position.GetOpenOrderId());
}
