	/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Instrument.hpp"
#include "Api.h"

class Position;
class Settings;

class TRADER_CORE_API Security
	: public Instrument,
	public boost::enable_shared_from_this<Security> {

public:

	typedef Instrument Base;

	typedef Trader::TradeSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

	typedef boost::posix_time::ptime MarketDataTime;

	typedef Trader::TradeSystem::OrderQty Qty;
	typedef Trader::TradeSystem::OrderPrice Price;

	struct TRADER_CORE_API Quote {
		
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
				boost::shared_ptr<Trader::TradeSystem>,
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

	~Security();

public:

	const Settings & GetSettings() const {
		return *m_settings;
	}

	const char * GetCurrency() const {
		return "USD";
	}

	unsigned int GetPriceScale() const throw() {
		return 100;
	}
	
	Price ScalePrice(double price) const {
		return Util::Scale(price, GetPriceScale());
	}

	double DescalePrice(Price price) const {
		return Util::Descale(price, GetPriceScale());
	}

public:

	boost::posix_time::ptime GetLastMarketDataTime() const;

	Trader::TradeSystem::OrderId SellAtMarketPrice(Qty, Position &);
	Trader::TradeSystem::OrderId Sell(Qty, Price, Position &);
	Trader::TradeSystem::OrderId SellAtMarketPriceWithStopPrice(Qty, Price stopPrice, Position &);
	Trader::TradeSystem::OrderId SellOrCancel(Qty, Price, Position &);

	Trader::TradeSystem::OrderId BuyAtMarketPrice(Qty, Position &);
	Trader::TradeSystem::OrderId Buy(Qty, Price, Position &);
	Trader::TradeSystem::OrderId BuyAtMarketPriceWithStopPrice(Qty, Price stopPrice, Position &);
	Trader::TradeSystem::OrderId BuyOrCancel(Qty, Price, Position &);

	void CancelOrder(Trader::TradeSystem::OrderId);
	void CancelAllOrders();

	bool IsCompleted() const;

public:

	//! Check security for valid market data and state.
	operator bool() const;

public:

	void ReportLevel2Snapshot(bool forced = false) const;

public:

	Price GetLastPriceScaled() const;
	double GetLastPrice() const;
	Qty GetLastSize() const {
		return m_lastSize;
	}
	
	Price GetAskPriceScaled() const;
	double GetAskPrice() const;
	Qty GetAskSize() const {
		return m_askSize;
	}
	
	Price GetBidPriceScaled() const;
	double GetBidPrice() const;
	Qty GetBidSize() const {
		return m_bidSize;
	}

	Qty GetLevel2AskSize();
	Qty GetLevel2BidSize();

private:

	void SetLastMarketDataTime(const boost::posix_time::ptime &);

	bool SetLast(double, Qty);
	
	bool SetAsk(double, Qty);
	bool SetBid(double, Qty);

	bool SetLast(Price, Qty);
	bool SetAsk(Price, Qty);
	bool SetBid(Price, Qty);

	void SetLevel2Ask(Qty askSize);
	void SetLevel2Bid(Qty bidSize);

public:

	bool IsHistoryData() const;

public:

	void UpdateLevel1(
				const MarketDataTime &timeOfReception,
				const MarketDataTime &lastTradeTime,
				double lastPrice,
				size_t lastSize,
				double askPrice,
				size_t askSize,
				double bidPrice,
				size_t bidSize,
				size_t totalVolume);

	void UpdateLevel2Line(
				const MarketDataTime &timeOfReception,
				int position,
				bool isAsk,
				double price,
				Qty size);
	void DeleteLevel2Line(
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
	MarketDataLog *m_marketDataLevel1Log;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;

	volatile LONGLONG m_isHistoryData;

	volatile LONGLONG m_lastPrice;
	volatile long m_lastSize;
	
	volatile LONGLONG m_askPrice;
	volatile long m_askSize;
	
	volatile LONGLONG m_bidPrice;
	volatile long m_bidSize;

	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	MarketDataTime m_marketDataTime;

};
