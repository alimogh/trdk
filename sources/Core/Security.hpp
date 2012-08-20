	/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Instrument.hpp"

class Position;
class Settings;

class Security
	: public Instrument,
	public boost::enable_shared_from_this<Security> {

public:

	typedef Instrument Base;

	typedef TradeSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

	typedef boost::posix_time::ptime MarketDataTime;

	typedef TradeSystem::OrderQty Qty;
	typedef TradeSystem::OrderPrice Price;

	struct Quote {
		
		unsigned int timeTick;
		double price;
		Qty size;

		Quote();

	};

	typedef void (UpdateSlotSignature)();
	typedef boost::function<UpdateSlotSignature> UpdateSlot;
	typedef SignalConnection<UpdateSlot, boost::signals2::connection> UpdateSlotConnection;

private:

	struct Level2 {
		
		volatile LONGLONG size;

		Level2();

	};

	struct QuotesAccumulated {
		typedef std::multimap<unsigned int, boost::shared_ptr<Quote>> Ticks;
		Ticks ask;
		Ticks bid;
		Level2 totalAsk;
		Level2 totalBid;
	};

	struct QuotesCompleted {
		typedef std::map<Price, Qty> Lines;
		Lines ask;
		Lines bid;
		Level2 totalAsk;
		Level2 totalBid;
	};
	

	typedef boost::shared_mutex MarketDataTimeMutex;
	typedef boost::shared_lock<MarketDataTimeMutex> MarketDataTimeReadLock;
	typedef boost::unique_lock<MarketDataTimeMutex> MarketDataTimeWriteLock;

	typedef boost::shared_mutex Level2Mutex;
	typedef boost::shared_lock<Level2Mutex> Level2ReadLock;
	typedef boost::unique_lock<Level2Mutex> Level2WriteLock;

public:

	explicit Security(
				boost::shared_ptr<TradeSystem>,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings,
				bool logMarketData);
	explicit Security(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings,
				bool logMarketData);

public:

	const Settings & GetSettings() const {
		return *m_settings;
	}

	const char * GetCurrency() const {
		return "USD";
	}

	unsigned int GetScale() const throw() {
		return 100;
	}
	
	Price Scale(double price) const {
		return Util::Scale(price, GetScale());
	}

	double Descale(Price price) const {
		return Util::Descale(price, GetScale());
	}

public:

	boost::posix_time::ptime GetLastMarketDataTime() const;

	TradeSystem::OrderId SellAtMarketPrice(Qty, Position &);
	TradeSystem::OrderId Sell(Qty, Price, Position &);
	TradeSystem::OrderId SellAtMarketPriceWithStopPrice(Qty, Price stopPrice, Position &);
	TradeSystem::OrderId SellOrCancel(Qty, Price, Position &);

	TradeSystem::OrderId BuyAtMarketPrice(Qty, Position &);
	TradeSystem::OrderId Buy(Qty, Price, Position &);
	TradeSystem::OrderId BuyAtMarketPriceWithStopPrice(Qty, Price stopPrice, Position &);
	TradeSystem::OrderId BuyOrCancel(Qty, Price, Position &);

	void CancelOrder(TradeSystem::OrderId);
	void CancelAllOrders();

	bool IsCompleted() const;

public:

	//! Check security for valid market data and state.
	operator bool() const;

public:

	void ReportLevel2Snapshot(bool forced = false) const;

public:

	Price GetLastPriceScaled() const;
	Price GetAskPriceScaled() const;
	Price GetBidPriceScaled() const;
	
	double GetLastPrice() const;
	double GetAskPrice() const;
	double GetBidPrice() const;

	Qty GetLevel2AskSizeIqFeed();
	Qty GetLevel2AskSizeIb();
	Qty GetLevel2BidSizeIqFeed();
	Qty GetLevel2BidSizeIb();

private:

	void SetLastMarketDataTime(const boost::posix_time::ptime &);

	bool SetLastPrice(double);
	
	bool SetAskPrice(double);
	bool SetBidPrice(double);

	bool SetLastPrice(Price);
	bool SetAskPrice(Price);
	bool SetBidPrice(Price);

	void SetLevel2AskIqFeed(Qty askSize);
	void SetLevel2AskIb(Qty askSize);
	void SetLevel2BidIqFeed(Qty bidSize);
	void SetLevel2BidIb(Qty bidSize);

public:

	bool IsHistoryData() const;

public:

	void UpdateLevel1(
				const MarketDataTime &timeOfReception,
				const MarketDataTime &lastTradeTime,
				double last,
				double ask,
				double bid,
				size_t totalVolume);
	void UpdateLevel2IqFeed(
				const MarketDataTime &timeOfReception,
				boost::shared_ptr<Quote> ask,
				boost::shared_ptr<Quote> bid);

	void UpdateLevel2IbLine(
				const MarketDataTime &timeOfReception,
				int position,
				bool isAsk,
				double price,
				Qty size);
	void DeleteLevel2IbLine(
				const MarketDataTime &timeOfReception,
				int position,
				bool isAsk,
				double price,
				Qty size);

	void OnHistoryDataStart();
	void OnHistoryDataEnd();

public:

	UpdateSlotConnection Subcribe(const UpdateSlot &) const;

private:

	boost::shared_ptr<const Settings> m_settings;

	class MarketDataLog;
	std::unique_ptr<MarketDataLog> m_marketDataLevel1Log;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;

	volatile LONGLONG m_isHistoryData;

	volatile LONGLONG m_last;
	volatile LONGLONG m_ask;
	volatile LONGLONG m_bid;

	class MarketDataLevel2Log;
	std::unique_ptr<MarketDataLevel2Log> m_marketDataLevel2Log;

	class MarketDataLevel2SnapshotLog;
	std::unique_ptr<MarketDataLevel2SnapshotLog> m_marketDataLevel2SnapshotLog;

	mutable Level2Mutex m_level2Mutex;
	QuotesAccumulated m_qoutesIqFeed;
	QuotesCompleted m_qoutesIb;
	
	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	MarketDataTime m_marketDataTime;

};
