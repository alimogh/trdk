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
			<< "," << (lastTradeTime + Util::GetEdtDiff()).time_of_day()
			<< "," << (timeOfReception - lastTradeTime).total_seconds()
			<< "," << last
			<< "," << ask
			<< "," << bid
			<< ","
			<< totalVolume
			<< std::endl;
	}

private:

	std::ofstream m_file;

};

class Security::MarketDataLevel2Log : private boost::noncopyable {

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
			m_file << "time of reception,tick time,lag,is skipped,lines count,type,price,size,tick size,first tick time" << std::endl;
		}
	}

public:

	void AppendAsk(
				const MarketDataTime &timeOfReception,
				unsigned int timeTick,
				double price,
				size_t size,
				boost::int64_t tickSize,
				unsigned int firstTickSize,
				bool isSkipped,
				size_t linesCount) {
		Append(
			"ask",
			timeOfReception,
			timeTick,
			price,
			size,
			tickSize,
			firstTickSize,
			isSkipped,
			linesCount);
	}

	void AppendBid(
				const MarketDataTime &timeOfReception,
				unsigned int timeTick,
				double price,
				size_t size,
				boost::int64_t tickSize,
				unsigned int firstTickSize,
				bool isSkipped,
				size_t linesCount) {
		Append(
			"bid",
			timeOfReception,
			timeTick,
			price,
			size,
			tickSize,
			firstTickSize,
			isSkipped,
			linesCount);
	}

private:

	void Append(
				const char *tag,
				const MarketDataTime &timeOfReception,
				unsigned int timeTick,
				double price,
				size_t size,
				boost::int64_t tickSize,
				unsigned int firstTickSize,
				bool isSkipped,
				size_t linesCount) {
		const pt::ptime timeOfReceptionEdt = timeOfReception + Util::GetEdtDiff();
		pt::ptime timeTickFull = timeOfReceptionEdt - timeOfReceptionEdt.time_of_day();
		AddTime(timeTick, timeTickFull);
		m_file
			<< timeOfReceptionEdt.time_of_day()
			<< ",";
		DumpTime(timeTick);
		m_file
			<< "," << (timeOfReceptionEdt - timeTickFull).total_seconds()
			<< "," << (isSkipped ? "skipped" : "-")
			<< "," << linesCount
			<< "," << tag
			<< "," << price
			<< "," << size
			<< "," << tickSize
			<< ",";
		DumpTime(firstTickSize);
		m_file << std::endl;
	}

private:

	void DumpTime(unsigned int time) {
		const size_t hours = time / 10000;
		const size_t minutes = (time - (hours * 10000)) / 100;
		const size_t seconds = time - (hours * 10000) - (minutes * 100);
		m_file << hours << ":" << minutes << ":" << seconds;
	}

	static void AddTime(unsigned int source, pt::ptime &destination) {
		const size_t hours = source / 10000;
		const size_t minutes = (source - (hours * 10000)) / 100;
		const size_t seconds = source - (hours * 10000) - (minutes * 100);
		destination += pt::hours(hours);
		destination += pt::minutes(minutes);
		destination += pt::seconds(seconds);
	}

private:

	std::ofstream m_file;

};

class Security::MarketDataLevel2SnapshotLog : private boost::noncopyable {

private:

	typedef std::map<Price, std::pair<Qty, Qty>, std::greater<Price>> Book;

public:

	explicit MarketDataLevel2SnapshotLog(
				const std::string &fullSymbol,
				unsigned int scale,
				const Settings &settings)
			: m_scale(scale),
			m_settings(settings) {
		OpenFile(fullSymbol, "IQFeed", m_fileIqFeed);
		OpenFile(fullSymbol, "IB", m_fileIb);
		Interlocking::Exchange(m_lastShapshotTime, 0);
	}

public:

	bool AppendCurrentIqFeed(
				const pt::ptime timeStamp,
				const QuotesAccumulated &quotes,
				bool force) {
		return Append(timeStamp, quotes, force, m_fileIqFeed);
	}

	bool AppendCurrentIb(
				const pt::ptime timeStamp,
				const QuotesCompleted &quotes,
				bool force) {
		return Append(timeStamp, quotes, force, m_fileIb);
	}

private:

	template<typename Quotes>
	bool Append(
				const pt::ptime &timeStamp,
				const Quotes &quotes,
				bool force,
				std::ostream &out) {

		if (!force) {
			const auto nowSecondsFromStart
				= (timeStamp - m_settings.GetStartTime()).total_seconds();
			if (!m_lastShapshotTime) {
				Interlocking::Exchange(m_lastShapshotTime, nowSecondsFromStart);
				return false;
			} else if (
					m_lastShapshotTime + m_settings.GetLevel2SnapshotPrintTimeSeconds()
						> nowSecondsFromStart) {
				return false;
			}
			m_lastShapshotTime = nowSecondsFromStart;
		}

		const auto tabWith = 9;
		const auto sizeColumnWidth = 10;
		const auto priceColumnWidth = 6;

		Book book;
		CreateBook(quotes.ask, book, true);
		CreateBook(quotes.bid, book, false);

		out
			<< (timeStamp + Util::GetEdtDiff()).time_of_day()
			<< " (bids: " << quotes.bid.size()
			<< ", asks: " << quotes.ask.size()			
			<< ", prices: " << book.size() << "):"
			<< std::endl
			<< std::setw(tabWith + sizeColumnWidth)
			<< "BIDS:"
			<< " | " << std::setw(priceColumnWidth) << " " << " | "
			<< "ASKS:"
			<< std::endl;

		Qty sizeAskSum = 0;
		Qty sizeBidSum = 0;
		foreach (const auto &line, book) {
			out.flags(std::ios::right);
			out << std::setw(tabWith + sizeColumnWidth);
			if (line.second.second) {
				sizeBidSum += line.second.second;
				out << line.second.second;
			} else {
				out << " ";
			}
			out << " | ";
			out.flags(std::ios::internal);
			out
				<< std::setw(priceColumnWidth)
				<< Util::Descale(line.first, m_scale)
				<< " | ";
			out.flags(std::ios::left);
			if (line.second.first) {
				sizeAskSum += line.second.first;
				out << line.second.first;
			}
			out << std::endl;
		}

		out.flags(std::ios::right);
		out
			<< " TOTALS: "
			<< std::setw(sizeColumnWidth)
			<< sizeBidSum
			<< " | " << std::setw(priceColumnWidth) << " " << " | "
			<< sizeAskSum
			<< std::endl
			<< std::endl;
				
		return true;

	}

	void OpenFile(
				const std::string &fullSymbol,
				const std::string &source,
				std::ofstream &file)  {
		const fs::path filePath = Util::SymbolToFilePath(
			Defaults::GetMarketDataLogDir(),
			fullSymbol + ":Level2:" + source,
			"snapshot");
		const bool isNew = !fs::exists(filePath);
		if (isNew) {
			fs::create_directories(filePath.branch_path());
		}
		file.open(filePath.c_str(), std::ios::out | std::ios::ate | std::ios::app);
		if (!file) {
			Log::Error(
				"Failed to open log file %1% for security market data snapshots.",
				filePath);
			throw Exception("Failed to open log file for security market data snapshots");
		}
		Log::Info("Logging \"%1%\" market data snapshots into %2%...", fullSymbol, filePath);
	}

private:

	void CreateBook(const QuotesAccumulated::Ticks &quotes, Book &book, bool isAsk) {
		foreach (const auto &line, quotes) {
			const Price price = Util::Scale(line.second->price, m_scale);
			const Book::iterator i = book.find(price);
			if (i == book.end()) {
				book.insert(
					std::make_pair(
						price,
						isAsk
							?	std::make_pair(line.second->size, 0)
							:	std::make_pair(0, line.second->size)));
			} else {
				(isAsk ? i->second.first : i->second.second) += line.second->size;
			}
		}
	}

	void CreateBook(const QuotesCompleted::Lines &quotes, Book &book, bool isAsk) {
		foreach (const auto &line, quotes) {
			const Price price = line.first;
			const Book::iterator i = book.find(price);
			if (i == book.end()) {
				book.insert(
					std::make_pair(
						price,
						isAsk
							?	std::make_pair(line.second, 0)
							:	std::make_pair(0, line.second)));
			} else {
				(isAsk ? i->second.first : i->second.second) += line.second;
			}
		}
	}


private:

	unsigned int m_scale;
	const Settings &m_settings;
	std::ofstream m_fileIqFeed;
	std::ofstream m_fileIb;
	volatile LONGLONG m_lastShapshotTime;


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
		m_marketDataLevel2SnapshotLog.reset(
			new MarketDataLevel2SnapshotLog(GetFullSymbol(), GetScale(), *m_settings));
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
		m_marketDataLevel2SnapshotLog.reset(
			new MarketDataLevel2SnapshotLog(GetFullSymbol(), GetScale(), *m_settings));
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
				double last,
				double ask,
				double bid,
				size_t totalVolume) {
	if (m_marketDataLevel1Log) {
		m_marketDataLevel1Log->Append(
			timeOfReception,
			lastTradeTime,
			last,
			ask,
			bid,
			totalVolume);
	}
	SetLastMarketDataTime(timeOfReception);
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


void Security::UpdateLevel2IqFeed(
			const MarketDataTime &timeOfReception,
			boost::shared_ptr<Quote> ask,
			boost::shared_ptr<Quote> bid) {
	Assert(ask || bid);
	auto isAskSkipped = true;
	auto isBidSkipped = true;
	{
		const Level2WriteLock lock(m_level2Mutex);
		if (ask) {
			Qty change = 0;
			isAskSkipped = !UpdateQuoteList(*m_settings, GetAskSizeIqFeed(), ask, m_qoutesIqFeed.ask, change);
			Assert(!isAskSkipped || change == 0);
			if (!isAskSkipped) {
				SetLevel2AskIqFeed(GetAskSizeIqFeed() + change);
			}
		}
		if (bid) {
			Qty change = 0;
			isBidSkipped = !UpdateQuoteList(*m_settings, GetBidSizeIqFeed(), bid, m_qoutesIqFeed.bid, change);
			Assert(!isBidSkipped || change == 0);
			if (!isBidSkipped) {
				SetLevel2BidIqFeed(GetBidSizeIqFeed() + change);
			}
		}
	}
	if (m_marketDataLevel2Log) {
		// no read locking needed - only this thread can change storage
		if (ask && !m_qoutesIqFeed.ask.empty()) {
			m_marketDataLevel2Log->AppendAsk(
				timeOfReception,
				ask->timeTick,
				ask->price,
				ask->size,
				GetAskSizeIqFeed(),
				m_qoutesIqFeed.ask.begin()->second->timeTick,
				isAskSkipped,
				m_qoutesIqFeed.ask.size());
		}
		if (bid && !m_qoutesIqFeed.bid.empty()) {
			m_marketDataLevel2Log->AppendBid(
				timeOfReception,
				bid->timeTick,
				bid->price,
				bid->size,
				GetBidSizeIqFeed(),
				m_qoutesIqFeed.bid.begin()->second->timeTick,
				isBidSkipped,
				m_qoutesIqFeed.bid.size());
		}
	}
	if (*this) {
		m_updateSignal();
	}
}

namespace {

	template<typename Quotes>
	Security::Qty GetQuotesSize(const Quotes &quotes) {
		Security::Qty result = 0;
		foreach (auto &q, quotes) {
			result += q.second;
		}
		return result;
	}

}

void Security::UpdateLevel2IbLine(
			const MarketDataTime &/*timeOfReception*/,
			int /*position*/,
			bool isAsk,
			double price,
			Qty size) {
	{
		const Level2WriteLock lock(m_level2Mutex);
		if (isAsk) {
			if (	!m_qoutesIb.ask.empty()
					&& (m_qoutesIb.totalAsk.size / m_qoutesIb.ask.size()) * 5 < size) {
				return;
			}
			m_qoutesIb.ask[Scale(price)] = size;
			SetLevel2AskIb(GetQuotesSize(m_qoutesIb.ask));
		} else {
			if (	!m_qoutesIb.bid.empty()
					&& (m_qoutesIb.totalBid.size / m_qoutesIb.bid.size()) * 5 < size) {
				return;
			}
			m_qoutesIb.bid[Scale(price)] = size;
			SetLevel2BidIb(GetQuotesSize(m_qoutesIb.bid));
		}
	}
	if (*this) {
		m_updateSignal();
	}
}

void Security::DeleteLevel2IbLine(
			const MarketDataTime &/*timeOfReception*/,
			int /*position*/,
			bool isAsk,
			double price,
			Qty size) {
	UseUnused(size);
	{
		const Level2WriteLock lock(m_level2Mutex);
		QuotesCompleted::Lines &quotes = isAsk ? m_qoutesIb.ask : m_qoutesIb.bid;
		const auto priceScaled = Scale(price);
		Assert(
			quotes.find(priceScaled) == quotes.end()
			|| quotes.find(priceScaled)->second == size);
		quotes.erase(priceScaled);
		if (isAsk) {
			SetLevel2AskIb(GetQuotesSize(quotes));
		} else {
			SetLevel2BidIb(GetQuotesSize(quotes));
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

Security::Price Security::GetBidScaled() const {
	return m_bid;
}

double Security::GetLast() const {
	return Descale(GetLastScaled());
}

double Security::GetAsk() const {
	return Descale(GetAskScaled());
}

double Security::GetBid() const {
	return Descale(GetBidScaled());
}

Security::Qty Security::GetAskSizeIqFeed() {
	return Security::Qty(m_qoutesIqFeed.totalAsk.size);
}

Security::Qty Security::GetBidSizeIqFeed() {
	return Security::Qty(m_qoutesIqFeed.totalBid.size);
}

Security::Qty Security::GetAskSizeIb() {
	return Security::Qty(m_qoutesIb.totalAsk.size);
}

Security::Qty Security::GetBidSizeIb() {
	return Security::Qty(m_qoutesIb.totalBid.size);
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

void Security::SetLevel2AskIqFeed(Qty askSize) {
	Interlocking::Exchange(m_qoutesIqFeed.totalAsk.size, askSize);
}

void Security::SetLevel2BidIqFeed(Qty bidSize) {
	Interlocking::Exchange(m_qoutesIqFeed.totalBid.size, bidSize);
}

void Security::SetLevel2AskIb(Qty askSize) {
	Interlocking::Exchange(m_qoutesIb.totalAsk.size, askSize);
}

void Security::SetLevel2BidIb(Qty bidSize) {
	Interlocking::Exchange(m_qoutesIb.totalBid.size, bidSize);
}

void Security::ReportLevel2Snapshot() const {
	if (!m_marketDataLevel2SnapshotLog || !m_settings->IsLevel2SnapshotPrintEnabled()) {
		return;
	}
	const Level2ReadLock lock(m_level2Mutex);
	const auto now = boost::get_system_time();
	const auto isIqFeedPrinted = m_marketDataLevel2SnapshotLog->AppendCurrentIqFeed(
		now,
		m_qoutesIqFeed,
		false);
	const auto isIbPrinted = m_marketDataLevel2SnapshotLog->AppendCurrentIb(
		now,
		m_qoutesIb,
		isIqFeedPrinted);
	if (isIbPrinted && !isIqFeedPrinted) {
		Verify(
			m_marketDataLevel2SnapshotLog->AppendCurrentIqFeed(
				now,
				m_qoutesIqFeed,
				true));
	} else {
		Assert((isIbPrinted && isIqFeedPrinted) || (!isIbPrinted && !isIqFeedPrinted));
	}
}
