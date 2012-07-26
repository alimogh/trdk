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

	typedef std::multimap<unsigned int, boost::shared_ptr<Quote>> Quotes;

	struct Level2 {
		
		volatile LONGLONG price;
		volatile LONGLONG size;

		Level2();

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

	void Sell(Qty, Position &);
	void Sell(Qty, Price, Position &);
	void SellAtMarketPrice(Qty, Price stopPrice, Position &);
	void SellOrCancel(Qty, Price, Position &);

	void Buy(Qty, Position &);
	void Buy(Qty, Price, Position &);
	void BuyAtMarketPrice(Qty, Price stopPrice, Position &);
	void BuyOrCancel(Qty, Price, Position &);

	void CancelAllOrders();

	bool IsCompleted() const;

public:

	//! Check security for valid market data and state.
	operator bool() const;

public:

	void ReportLevel2Snapshot() const;

public:

	Price GetLastScaled() const;
	Price GetAskScaled() const;
	Price GetAskLevel2Scaled() const;
	Price GetBidScaled() const;
	Price GetBidLevel2Scaled() const;
	
	double GetLast() const;
	double GetAsk() const;
	double GetAskLevel2() const;
	double GetBid() const;
	double GetBidLevel2() const;

	Qty GetAskSize();
	Qty GetBidSize();

private:

	void SetLastMarketDataTime(const boost::posix_time::ptime &);

	bool SetLast(double);
	
	bool SetAsk(double);
	bool SetBid(double);

	bool SetLast(Price);
	bool SetAsk(Price);
	bool SetBid(Price);

	void SetLevel2Ask(Price askPrice, Qty askSize);
	void SetLevel2Bid(Price bidPrice, Qty bidSize);

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
	void UpdateLevel2(
				const MarketDataTime &timeOfReception,
				boost::shared_ptr<Quote> ask,
				boost::shared_ptr<Quote> bid);

	void OnHistoryDataStart();
	void OnHistoryDataEnd();

public:

	UpdateSlotConnection Subcribe(const UpdateSlot &) const;

private:

	boost::shared_ptr<const Settings> m_settings;

	class MarketDataLog;
	std::unique_ptr<MarketDataLog> m_marketDataLevel1Log;

	class MarketDataLevel2Log;
	std::unique_ptr<MarketDataLevel2Log> m_marketDataLevel2Log;

	class MarketDataLevel2SnapshotLog;
	std::unique_ptr<MarketDataLevel2SnapshotLog> m_marketDataLevel2SnapshotLog;
	mutable Level2Mutex m_level2Mutex;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;

	volatile LONGLONG m_isHistoryData;

	volatile LONGLONG m_last;
	volatile LONGLONG m_ask;
	volatile LONGLONG m_bid;

	Quotes m_askQoutes;
	Level2 m_askLevel2;
	Quotes m_bidQoutes;
	Level2 m_bidLevel2;

	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	MarketDataTime m_marketDataTime;

};
