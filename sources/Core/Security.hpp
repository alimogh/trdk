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

public:

	explicit Security(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange);

public:

	//! Check security for valid market data and state.
	operator bool() const;

public:

	ScaledPrice GetLastScaled() const;
	ScaledPrice GetAskScaled() const;
	ScaledPrice GetBidScaled() const;

protected:

	bool SetLast(Price last);
	bool SetAsk(Price ask);
	bool SetBid(Price bid);

	bool SetLast(ScaledPrice);
	bool SetAsk(ScaledPrice);
	bool SetBid(ScaledPrice);

private:

	volatile LONGLONG m_last;
	volatile LONGLONG m_ask;
	volatile LONGLONG m_bid;

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
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				bool logMarketData);

public:

	bool IsHistoryData() const {
		return m_isHistoryData ? true : false;
	}

public:

	void Update(const MarketDataTime &, Price last, Price ask, Price bid);

	void OnHistoryDataStart();
	void OnHistoryDataEnd();

public:

	UpdateSlotConnection Subcribe(const UpdateSlot &) const;

private:

	class MarketDataLog;
	std::unique_ptr<MarketDataLog> m_marketDataLog;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;

	volatile LONGLONG m_isHistoryData;

};

////////////////////////////////////////////////////////////////////////////////
