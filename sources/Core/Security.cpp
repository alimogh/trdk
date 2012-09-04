/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Position.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;

using namespace Trader;

//////////////////////////////////////////////////////////////////////////

class Security::MarketDataLog : private boost::noncopyable {

public:

	explicit MarketDataLog(const std::string &fullSymbol) {
		const fs::path filePath = Util::SymbolToFilePath(
			Defaults::GetMarketDataLogDir(),
			fullSymbol,
			"csv");
		const bool isNew = !fs::exists(filePath);
		if (isNew) {
			fs::create_directories(filePath.branch_path());
		}
		m_file.open(filePath.c_str(), std::ios::out | std::ios::ate | std::ios::app);
		if (!m_file) {
			Log::Error(
				"Failed to open log file %1% for security market data.",
				filePath);
			throw Exception("Failed to open log file for security market data");
		}
		Log::Info("Logging \"%1%\" market data into %2%...", fullSymbol, filePath);
		if (isNew) {
			m_file << "time of reception,last trade time,lag,last price,ask,bid,total volume" << std::endl;
		}
	}

public:

	void Append(
				const MarketDataTime &timeOfReception,
				const MarketDataTime &lastTradeTime,
				double last,
				double ask,
				double bid,
				size_t totalVolume) {
		m_file
			<< (timeOfReception + Util::GetEdtDiff()).time_of_day()
			<< ',' << (lastTradeTime + Util::GetEdtDiff()).time_of_day()
			<< ',' << (timeOfReception - lastTradeTime).total_seconds()
			<< ',' << last
			<< ',' << ask
			<< ',' << bid
			<< ','
			<< totalVolume
			<< std::endl;
	}

private:

	std::ofstream m_file;

};

//////////////////////////////////////////////////////////////////////////

Security::Security(
				boost::shared_ptr<TradeSystem> tradeSystem,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings,
				bool logMarketData)
		: Base(tradeSystem, symbol, primaryExchange, exchange),
		m_settings(settings),
		m_marketDataLevel1Log(nullptr),
		m_isHistoryData(false),
		m_lastPrice(0),
		m_lastSize(0),
		m_askPrice(0),
		m_askSize(0),
		m_bidPrice(0),
		m_bidSize(0) {
	// see another ctor!
	if (logMarketData) {
		std::unique_ptr<MarketDataLog> marketDataLevel1Log(
			new MarketDataLog(GetFullSymbol()));
		m_marketDataLevel1Log = marketDataLevel1Log.release();
	}
}

Security::Security(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings,
				bool logMarketData)
		: Base(symbol, primaryExchange, exchange),
		m_settings(settings),
		m_marketDataLevel1Log(nullptr),
		m_lastPrice(0),
		m_lastSize(0),
		m_askPrice(0),
		m_askSize(0),
		m_bidPrice(0),
		m_bidSize(0) {
	// see another ctor!
	if (logMarketData) {
		std::unique_ptr<MarketDataLog> marketDataLevel1Log(
			new MarketDataLog(GetFullSymbol()));
		m_marketDataLevel1Log = marketDataLevel1Log.release();
	}
}

Security::~Security() {
	delete m_marketDataLevel1Log;
}

TradeSystem::OrderId Security::SellAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().SellAtMarketPrice(*this, qty, position.GetSellOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::Sell(Qty qty, Price price, Position &position) {
	return GetTradeSystem().Sell(*this, qty, price, position.GetSellOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::SellAtMarketPriceWithStopPrice(Qty qty, Price stopPrice, Position &position) {
	return GetTradeSystem().SellAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetSellOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::SellOrCancel(Qty qty, Price price, Position &position) {
	return GetTradeSystem().SellOrCancel(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::BuyAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().BuyAtMarketPrice(
		*this,
		qty,
		position.GetBuyOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::Buy(Qty qty, Price price, Position &position) {
	return GetTradeSystem().Buy(*this, qty, price, position.GetBuyOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::BuyAtMarketPriceWithStopPrice(
			Qty qty,
			Price stopPrice,
			Position &position) {
	return GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetBuyOrderStatusUpdateSlot());
}

TradeSystem::OrderId Security::BuyOrCancel(Qty qty, Price price, Position &position) {
	return GetTradeSystem().BuyOrCancel(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

void Security::CancelOrder(TradeSystem::OrderId orderId) {
	GetTradeSystem().CancelOrder(orderId);
}

void Security::CancelAllOrders() {
	GetTradeSystem().CancelAllOrders(*this);
}

bool Security::IsCompleted() const {
	return GetTradeSystem().IsCompleted(*this);
}

//////////////////////////////////////////////////////////////////////////

Security::Quote::Quote()
		: timeTick(0),
		price(0),
		size(0) {
	//...//
}

Security::Level2::Level2() {
	Interlocking::Exchange(size, 0);
}

bool Security::IsHistoryData() const {
	return m_isHistoryData
		?	!m_settings->IsReplayMode()
		:	false;
}

void Security::UpdateLevel1(
				const MarketDataTime &timeOfReception,
				const MarketDataTime &lastTradeTime,
				double lastPrice,
				size_t lastSize,
				double askPrice,
				size_t askSize,
				double bidPrice,
				size_t bidSize,
				size_t totalVolume) {
	if (m_marketDataLevel1Log) {
		m_marketDataLevel1Log->Append(
			timeOfReception,
			lastTradeTime,
			lastPrice,
			askPrice,
			bidPrice,
			totalVolume);
	}
	SetLastMarketDataTime(timeOfReception);
	if (	!SetLast(lastPrice, lastSize)
			|| !SetAsk(askPrice, askSize)
			|| !SetBid(bidPrice, bidSize)) {
		if (!m_settings->IsReplayMode()) {
			return;
		} else {
			m_updateSignal();
		}
	}
	Assert(*this);
	if (!*this) {
		return;
	}
	m_updateSignal();
}


namespace {

	template<typename List>
	bool UpdateQuoteList(
				const Settings &settings,
				const Security::Qty currentSize,
				boost::shared_ptr<Security::Quote> &newQuote,
				List &quoteList,
				Security::Qty &change) {
		Assert(newQuote->timeTick <= 235959);
		Assert(newQuote->timeTick > 0);
		auto isChanged = false;
		if (	!quoteList.empty()
				&& (newQuote->timeTick
					< quoteList.rbegin()->second->timeTick - settings.GetLevel2PeriodSeconds()
					|| Security::Qty(currentSize / quoteList.size()) * 5 < newQuote->size)) {
			change = 0;
			return isChanged;
		}
		auto result = newQuote->size;
		isChanged = true;
		quoteList.insert(std::make_pair(newQuote->timeTick, newQuote));
		while (!quoteList.empty()) {
			const auto &first = *quoteList.begin()->second;
			const auto &last = *quoteList.rbegin()->second;
			Assert(first.timeTick <= 235959);
			Assert(first.timeTick >= 0);
			Assert(first.timeTick <= last.timeTick);
			const auto isNewDay = !(first.timeTick <= last.timeTick);
			const auto period = isNewDay
				?	(235959 - first.timeTick) + last.timeTick
				:	last.timeTick - first.timeTick;
			if (period <= settings.GetLevel2PeriodSeconds()) {
				break;
			}
			result -= first.size;
			quoteList.erase(quoteList.begin());
		}
		change = result;
		return isChanged;
	}

}

void Security::UpdateLevel2Line(
			const MarketDataTime &/*timeOfReception*/,
			int /*position*/,
			bool /*isAsk*/,
			double /*price*/,
			Qty /*size*/) {
	AssertFail("Doesn't implemented.");
}

void Security::DeleteLevel2Line(
			const MarketDataTime &/*timeOfReception*/,
			int /*position*/,
			bool /*isAsk*/,
			double /*price*/,
			Qty /*size*/) {
	AssertFail("Doesn't implemented.");
}

Security::UpdateSlotConnection Security::Subcribe(
			const UpdateSlot &slot)
		const {
	return UpdateSlotConnection(m_updateSignal.connect(slot));
}

void Security::OnHistoryDataStart() {
	if (!Interlocking::Exchange(m_isHistoryData, true) && *this) {
		m_updateSignal();
	}
}

void Security::OnHistoryDataEnd() {
	if (Interlocking::Exchange(m_isHistoryData, false) && *this) {
		m_updateSignal();
	}
}

Security::operator bool() const {
	return m_lastPrice && m_askPrice && m_bidPrice;
}

Security::Price Security::GetLastPriceScaled() const {
	return m_lastPrice;
}

Security::Price Security::GetAskPriceScaled() const {
	return m_askPrice;
}

Security::Price Security::GetBidPriceScaled() const {
	return m_bidPrice;
}

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Security::Qty Security::GetLevel2AskSize() {
	AssertFail("Doesn't implemented.");
	return 0;
}

Security::Qty Security::GetLevel2BidSize() {
	AssertFail("Doesn't implemented.");
	return 0;
}

pt::ptime Security::GetLastMarketDataTime() const {
	Assert(m_settings->IsReplayMode());
	const MarketDataTimeReadLock lock(m_marketDataTimeMutex);
	Assert(!m_marketDataTime.is_not_a_date_time());
	return m_marketDataTime;
}

void Security::SetLastMarketDataTime(const boost::posix_time::ptime &time) {
	if (!m_settings->IsReplayMode()) {
		return;
	}
	const MarketDataTimeWriteLock lock(m_marketDataTimeMutex);
	m_marketDataTime = time;
}

bool Security::SetLast(double price, Qty size) {
	return SetLast(ScalePrice(price), size);
}

bool Security::SetAsk(double price, Qty size) {
	return SetAsk(ScalePrice(price), size);
}

bool Security::SetBid(double price, Qty size) {
	return SetBid(ScalePrice(price), size);
}

bool Security::SetLast(Price price, Qty size) {
	bool isChanged = Interlocking::Exchange(m_lastPrice, price) != price;
	isChanged = Interlocking::Exchange(m_lastSize, size) != size || isChanged;
	return isChanged;
}

bool Security::SetAsk(Price price, Qty size) {
	Assert((!price && !size) || (price && size));
	if (!price || !size) {
		return false;
	}
	bool isChanged = Interlocking::Exchange(m_askPrice, price) != price;
	isChanged = Interlocking::Exchange(m_askSize, size) != size || isChanged;
	return isChanged;
}

bool Security::SetBid(Price price, Qty size) {
	Assert((!price && !size) || (price && size));
	if (!price || !size) {
		return false;
	}
	bool isChanged = Interlocking::Exchange(m_bidPrice, price) != price;
	isChanged = Interlocking::Exchange(m_askSize, size) != size || isChanged;
	return isChanged;
}

void Security::SetLevel2Ask(Qty /*askSize*/) {
	AssertFail("Doesn't implemented.");
}

void Security::SetLevel2Bid(Qty /*bidSize*/) {
	AssertFail("Doesn't implemented.");
}

void Security::ReportLevel2Snapshot(bool /*forced*/ /*= false*/) const {
	AssertFail("Doesn't implemented.");
}
