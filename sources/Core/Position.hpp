/**************************************************************************
 *   Created: 2012/07/09 17:32:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "TradeSystem.hpp"

class Position : private boost::noncopyable {

public:

	typedef void (StateUpdateSlotSignature)();
	typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
	typedef SignalConnection<
			StateUpdateSlot,
			boost::signals2::connection>
		StateUpdateConnection;

	typedef TradeSystem::OrderPrice Price;
	typedef TradeSystem::OrderQty Qty;
	typedef boost::posix_time::ptime Time;

	enum Type {
		TYPE_LONG,
		TYPE_SHORT
	};

private:

	typedef boost::signals2::signal<StateUpdateSlotSignature> StateUpdateSignal;

public:

	explicit Position(
				Type,
				Price startPrice,
				Price decisionAks,
				Price decisionBid,
				Price takeProfit,
				Price stopLoss);
	~Position();

public:

	bool IsOpened() const;
	bool IsClosed() const;
	bool IsCanceled() const;
	bool IsCompleted() const;

	Type GetType() const;
	Price GetStartPrice() const;
	Price GetDecisionAks() const;
	Price GetDecisionBid() const;
	Price GetTakeProfit() const;
	Price GetStopLoss() const;

	Price GetOpenPrice() const;
	Price GetClosePrice() const;

public:

	StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

private:

	mutable StateUpdateSignal m_stateUpdateSignal;

private:

	const Type m_type;
	const Price m_startPrice;
	const Price m_decisionAks;
	const Price m_decisionBid;
	const Price m_takeProfit;
	const Price m_stopLoss;

	volatile LONGLONG m_openPrice;
	Time m_openTime;

	volatile LONGLONG m_closePrice;
	Time m_closeTime;

	volatile LONGLONG m_state;

};
