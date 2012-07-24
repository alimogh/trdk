/**************************************************************************
 *   Created: 2012/07/09 17:32:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "TradeSystem.hpp"

class Algo;

class Position
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Position> {

public:

	typedef void (StateUpdateSlotSignature)();
	typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
	typedef SignalConnection<
			StateUpdateSlot,
			boost::signals2::connection>
		StateUpdateConnection;

	typedef Security::Price Price;
	typedef Security::Qty Qty;
	typedef boost::posix_time::ptime Time;

	enum Type {
		TYPE_LONG,
		TYPE_SHORT
	};

	enum CloseType {
		CLOSE_TYPE_NONE,
		CLOSE_TYPE_TAKE_PROFIT,
		CLOSE_TYPE_STOP_LOSS
	};

	typedef boost::int64_t AlgoFlag;

private:

	typedef boost::signals2::signal<StateUpdateSlotSignature> StateUpdateSignal;

	struct DynamicData {

		TradeSystem::OrderId orderId;
		Time time;
		Price price;
		Qty qty;
		volatile LONGLONG comission;

		DynamicData();

	};

public:

	explicit Position(
			boost::shared_ptr<const Security> security,
			Type,
			Qty,
			Price startPrice,
			Price decisionAks,
			Price decisionBid,
			Price takeProfit,
			Price stopLoss,
			AlgoFlag algoFlag,
			boost::shared_ptr<const Algo> algo);
	~Position();

public:

	const Security & GetSecurity() const;

	void SetCloseType(CloseType);
	CloseType GetCloseType() const;
	const char * GetCloseTypeStr() const;
	
	bool IsReported() const;
	void MarkAsReported();

	AlgoFlag GetAlgoFlag() const;
	void SetAlgoFlag(AlgoFlag);

	bool IsOpened() const;
	bool IsClosed() const;
	bool IsOpenError() const;
	bool IsCloseError() const;
	bool IsCloseCanceled() const;

	void ResetState();

	Type GetType() const;
	const char * GetTypeStr() const;
	Qty GetPlanedQty() const;
	Price GetStartPrice() const;
	Price GetDecisionAks() const;
	Price GetDecisionBid() const;
	void SetTakeProfit(Price);
	Price GetTakeProfit() const;
	Price GetStopLoss() const;

	TradeSystem::OrderId GetOpenOrderId() const;
	Qty GetOpenedQty() const;
	Price GetOpenPrice() const;
	Time GetOpenTime() const;
	
	TradeSystem::OrderId GetCloseOrderId() const;
	Price GetClosePrice() const;
	Qty GetClosedQty() const;
	Time GetCloseTime() const;

	Price GetCommission() const;

public:

	StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

	Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
	Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

protected:

	void UpdateOpening(
				TradeSystem::OrderId,
				TradeSystem::OrderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice,
				double lastPrice);
	void UpdateClosing(
				TradeSystem::OrderId,
				TradeSystem::OrderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice,
				double lastPrice);

	void ReportOpeningUpdate(
				const char *eventDesc,
				TradeSystem::OrderStatus,
				long state)
			const;
	void ReportClosingUpdate(
				const char *eventDesc,
				TradeSystem::OrderStatus,
				long state)
			const;
	void ReportCloseOrderChange(
				TradeSystem::OrderStatus,
				long state,
				TradeSystem::OrderId prevOrderId,
				TradeSystem::OrderId newOrderId)
			const;

private:

	mutable StateUpdateSignal m_stateUpdateSignal;

	boost::shared_ptr<const Security> m_security;

	const Type m_type;
	const Qty m_planedQty;
	const Price m_startPrice;
	const Price m_decisionAks;
	const Price m_decisionBid;
	volatile LONGLONG m_takeProfit;
	const Price m_stopLoss;
		
	DynamicData m_opened;
	DynamicData m_closed;

	volatile long m_state;

	CloseType m_closeType;

	bool m_isReported;

	AlgoFlag m_algoFlag;
	boost::shared_ptr<const Algo> m_algo;

};
