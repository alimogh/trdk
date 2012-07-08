/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Instrument.hpp"

////////////////////////////////////////////////////////////////////////////////

class Security : public Instrument {

public:

	typedef Instrument Base;

	typedef TradeSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

	typedef boost::posix_time::ptime MarketDataTime;

	typedef TradeSystem::OrderQty Qty;
	typedef TradeSystem::OrderPrice Price;

public:

	explicit Security(
				boost::shared_ptr<TradeSystem>,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange);

public:

	const char * GetCurrency() const {
		return "USD";
	}

	unsigned int GetScale() const throw() {
		return 10000;
	}
	
	Price Scale(double price) const {
		return Util::Scale(price, GetScale());
	}

	double Descale(Price price) const {
		return Util::Descale(price, GetScale());
	}

public:

	void Sell(Qty, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void Sell(Qty, Price, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void SellAtMarketPrice(Qty, Price stopPrice, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void SellOrCancel(Qty, Price, OrderStatusUpdateSlot = OrderStatusUpdateSlot());

	void Buy(Qty, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void Buy(Qty, Price, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void BuyAtMarketPrice(Qty, Price stopPrice, OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	void BuyOrCancel(Qty, Price, OrderStatusUpdateSlot = OrderStatusUpdateSlot());

	void CancelAllOrders();

};

////////////////////////////////////////////////////////////////////////////////

class DynamicSecurity : public Security {

public:

	typedef Security Base;

	typedef void (UpdateSlotSignature)();
	typedef boost::function<UpdateSlotSignature> UpdateSlot;
	typedef SignalConnection<UpdateSlot, boost::signals2::connection> UpdateSlotConnection;

public:

	explicit DynamicSecurity(
				boost::shared_ptr<TradeSystem>,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				bool logMarketData);

public:

	//! Check security for valid market data and state.
	operator bool() const;

public:

	Price GetLastScaled() const;
	Price GetAskScaled() const;
	Price GetBidScaled() const;

protected:

	bool SetLast(double);
	bool SetAsk(double);
	bool SetBid(double);

	bool SetLast(Price);
	bool SetAsk(Price);
	bool SetBid(Price);

public:

	bool IsHistoryData() const {
		return m_isHistoryData ? true : false;
	}

public:

	void Update(const MarketDataTime &, double last, double ask, double bid);

	void OnHistoryDataStart();
	void OnHistoryDataEnd();

public:

	UpdateSlotConnection Subcribe(const UpdateSlot &) const;

private:

	class MarketDataLog;
	std::unique_ptr<MarketDataLog> m_marketDataLog;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;

	volatile LONGLONG m_isHistoryData;

	volatile LONGLONG m_last;
	volatile LONGLONG m_ask;
	volatile LONGLONG m_bid;

};

////////////////////////////////////////////////////////////////////////////////
