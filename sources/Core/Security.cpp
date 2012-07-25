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

//////////////////////////////////////////////////////////////////////////

class Security::MarketDataLog : private boost::noncopyable {

private:
		
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

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
			m_file << "data time,last price,ask,bid,total volume" << std::endl;
		} else {
			m_file << std::endl;
		}
	}

public:

	void Append(
				const MarketDataTime &time,
				double last,
				double ask,
				double bid,
				size_t totalVolume) {
		const lt::local_date_time esdTime(time, Util::GetEdtTimeZone());
		// const Lock lock(m_mutex); - not required while it uses only one IQLink thread
		m_file << esdTime << "," << last << "," << ask << "," << bid << "," << totalVolume << std::endl;
	}

private:

	// mutable Mutex m_mutex;
	std::ofstream m_file;

};

class Security::MarketDataLevel2Log : private boost::noncopyable {

private:
		
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

public:

	explicit MarketDataLevel2Log(const std::string &fullSymbol) {
		const fs::path filePath = Util::SymbolToFilePath(
			Defaults::GetMarketDataLogDir(),
			fullSymbol + ":Level2",
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
			m_file << "type,time tick, price,size,tick size,first tick time" << std::endl;
		} else {
			m_file << std::endl;
		}
	}

public:

	void AppendAsk(
				unsigned int timeTick,
				double price,
				size_t size,
				boost::int64_t tickSize,
				unsigned int firstTickSize) {
		m_file
			<< "ask,"
			<< timeTick
			<< "," << price
			<< "," << size
			<< "," << tickSize
			<< "," << firstTickSize
			<< std::endl;
	}

	void AppendBid(
				unsigned int timeTick,
				double price,
				size_t size,
				boost::int64_t tickSize,
				unsigned int firstTickSize) {
		m_file
			<< "bid,"
			<< timeTick
			<< "," << price
			<< "," << size
			<< "," << tickSize
			<< "," << firstTickSize
			<< std::endl;
	}

private:

	// mutable Mutex m_mutex;
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
		m_settings(settings) {
	if (logMarketData) {
		m_marketDataLevel1Log.reset(new MarketDataLog(GetFullSymbol()));
		m_marketDataLevel2Log.reset(new MarketDataLevel2Log(GetFullSymbol()));
	}
	Interlocking::Exchange(m_isHistoryData, false);
	Interlocking::Exchange(m_last, 0);
	Interlocking::Exchange(m_ask, 0);
	Interlocking::Exchange(m_bid, 0);
}

Security::Security(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings,
				bool logMarketData)
		: Base(symbol, primaryExchange, exchange),
		m_settings(settings) {
	if (logMarketData) {
		m_marketDataLevel1Log.reset(new MarketDataLog(GetFullSymbol()));
		m_marketDataLevel2Log.reset(new MarketDataLevel2Log(GetFullSymbol()));
	}
	Interlocking::Exchange(m_isHistoryData, false);
	Interlocking::Exchange(m_last, 0);
	Interlocking::Exchange(m_ask, 0);
	Interlocking::Exchange(m_bid, 0);
}

void Security::Sell(Qty qty, Position &position) {
	GetTradeSystem().Sell(*this, qty, position.GetSellOrderStatusUpdateSlot());
}

void Security::Sell(Qty qty, Price price, Position &position) {
	GetTradeSystem().Sell(*this, qty, price, position.GetSellOrderStatusUpdateSlot());
}

void Security::SellAtMarketPrice(Qty qty, Price stopPrice, Position &position) {
	GetTradeSystem().SellAtMarketPrice(*this, qty, stopPrice, position.GetSellOrderStatusUpdateSlot());
}

void Security::SellOrCancel(Qty qty, Price price, Position &position) {
	GetTradeSystem().SellOrCancel(*this, qty, price, position.GetSellOrderStatusUpdateSlot());
}

void Security::Buy(Qty qty, Position &position) {
	GetTradeSystem().Buy(*this, qty, position.GetBuyOrderStatusUpdateSlot());
}

void Security::Buy(Qty qty, Price price, Position &position) {
	GetTradeSystem().Buy(*this, qty, price, position.GetBuyOrderStatusUpdateSlot());
}

void Security::BuyAtMarketPrice(Qty qty, Price stopPrice, Position &position) {
	GetTradeSystem().BuyAtMarketPrice(*this, qty, stopPrice, position.GetBuyOrderStatusUpdateSlot());
}

void Security::BuyOrCancel(Qty qty, Price price, Position &position) {
	GetTradeSystem().BuyOrCancel(*this, qty, price, position.GetBuyOrderStatusUpdateSlot());
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
	Interlocking::Exchange(price, 0);
	Interlocking::Exchange(size, 0);
}

bool Security::IsHistoryData() const {
	return m_isHistoryData
		?	!m_settings->IsReplayMode()
		:	false;
}

void Security::UpdateLevel1(
				const MarketDataTime &time,
				double last,
				double ask,
				double bid,
				size_t totalVolume) {
	if (m_marketDataLevel1Log) {
		m_marketDataLevel1Log->Append(time, last, ask, bid, totalVolume);
	}
	SetLastMarketDataTime(time);
	if (!SetLast(last) || !SetAsk(ask) || !SetBid(bid)) {
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
	Security::Qty UpdateQuoteList(
				const Settings &settings,
				boost::shared_ptr<Security::Quote> &newQuote,
				List &quoteList) {
		Assert(newQuote->timeTick <= 235959);
		Assert(newQuote->timeTick > 0);
		Security::Qty result = newQuote->size;
		while (!quoteList.empty()) {
			const auto listTime = (**quoteList.begin()).timeTick;
			Assert(listTime <= 235959);
			Assert(listTime >= 0);
			const bool isNewDay = !(listTime <= newQuote->timeTick);
			const auto period = isNewDay
				?	(235959 - listTime) + newQuote->timeTick
				:	newQuote->timeTick - listTime;
			if (period <= settings.GetLevel2PeriodSeconds()) {
				break;
			}
			result -= (**quoteList.begin()).size;
			quoteList.pop_front();
		}
		quoteList.push_back(newQuote);
		return result;
	}

}


void Security::UpdateLevel2(boost::shared_ptr<Quote> ask, boost::shared_ptr<Quote> bid) {
	Assert(ask || bid);
	if (ask) {
		SetLevel2Ask(
			Scale(ask->price),
			GetAskSize() + UpdateQuoteList(*m_settings, ask, m_askQoutes));
		if (m_marketDataLevel2Log) {
			m_marketDataLevel2Log->AppendAsk(
				ask->timeTick,
				ask->price,
				ask->size,
				GetAskSize(),
				(**m_askQoutes.begin()).timeTick);
		}
	}
	if (bid) {
		SetLevel2Bid(
			Scale(bid->price),
			GetBidSize() + UpdateQuoteList(*m_settings, bid, m_bidQoutes));
		if (m_marketDataLevel2Log) {
			m_marketDataLevel2Log->AppendBid(
				bid->timeTick,
				bid->price,
				bid->size,
				GetBidSize(),
				(**m_bidQoutes.begin()).timeTick);
		}
	}
	if (*this) {
		m_updateSignal();
	}
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
	return m_last && m_ask && m_bid;
}

Security::Price Security::GetLastScaled() const {
	return m_last;
}

Security::Price Security::GetAskScaled() const {
	return m_ask;
}

Security::Price Security::GetAskLevel2Scaled() const {
	return m_askLevel2.price;
}

Security::Price Security::GetBidScaled() const {
	return m_bid;
}

Security::Price Security::GetBidLevel2Scaled() const {
	return m_bidLevel2.price;
}

double Security::GetLast() const {
	return Descale(GetLastScaled());
}

double Security::GetAsk() const {
	return Descale(GetAskScaled());
}

double Security::GetAskLevel2() const {
	return Descale(GetAskLevel2Scaled());
}

double Security::GetBid() const {
	return Descale(GetBidScaled());
}

double Security::GetBidLevel2() const {
	return Descale(GetBidLevel2Scaled());
}

Security::Qty Security::GetAskSize() {
	return Security::Qty(m_askLevel2.size);
}

Security::Qty Security::GetBidSize() {
	return Security::Qty(m_bidLevel2.size);
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

bool Security::SetLast(double last) {
	return SetLast(Scale(last));
}

bool Security::SetAsk(double ask) {
	return SetAsk(Scale(ask));
}

bool Security::SetBid(double bid) {
	return SetBid(Scale(bid));
}

bool Security::SetLast(Price last) {
	if (!last) {
		return false;
	}
	return Interlocking::Exchange(m_last, last) != last;
}

bool Security::SetAsk(Price ask) {
	if (!ask) {
		return false;
	}
	return Interlocking::Exchange(m_ask, ask) != ask;
}

bool Security::SetBid(Price bid) {
	if (!bid) {
		return false;
	}
	return Interlocking::Exchange(m_bid, bid) != bid;
}

void Security::SetLevel2Bid(Price bidPrice, Qty bidSize) {
	Interlocking::Exchange(m_bidLevel2.price, bidPrice);
	Interlocking::Exchange(m_bidLevel2.size, bidSize);
}

void Security::SetLevel2Ask(Price askPrice, Qty askSize) {
	Interlocking::Exchange(m_askLevel2.price, askPrice);
	Interlocking::Exchange(m_askLevel2.size, askSize);
}
