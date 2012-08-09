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
class AlgoPositionState;

//////////////////////////////////////////////////////////////////////////

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
		CLOSE_TYPE_STOP_LOSS,
		CLOSE_TYPE_TIME
	};

	typedef boost::int64_t AlgoFlag;

private:

	typedef boost::signals2::signal<StateUpdateSlotSignature> StateUpdateSignal;

	struct DynamicData {

		volatile LONGLONG orderId;
		Time time;
		Price price;
		Qty qty;
		volatile LONGLONG comission;

		DynamicData();

	};

public:

	explicit Position(
			boost::shared_ptr<Security> security,
			Qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
	virtual ~Position();

public:

	virtual Type GetType() const = 0;
	virtual const std::string & GetTypeStr() const = 0;

public:

	const Security & GetSecurity() const;
	Security & GetSecurity();

	const Algo & GetAlgo() const {
		return *m_algo;
	}

	template<typename AlogState>
	bool IsAlgoStateSet() const {
		return m_algoState && dynamic_cast<const AlogState *>(m_algoState.get());
	}

	template<typename AlogState>
	AlogState & GetAlgoState() {
		Assert(IsAlgoStateSet<AlogState>());
		return *boost::polymorphic_downcast<AlogState *>(m_algoState.get());
	}
	template<typename AlogState>
	const AlogState & GetAlgoState() const {
		return const_cast<Position *>(this)->GetAlgoState<AlogState>();
	}

public:

	void SetCloseType(CloseType);
	CloseType GetCloseType() const;
	const char * GetCloseTypeStr() const;

	bool IsReported() const;
	void MarkAsReported();

	bool IsOpened() const;
	bool IsClosed() const;
	bool IsOpenError() const;
	bool IsCloseError() const;
	bool IsCloseCanceled() const;

public:

	void ResetState();

	Qty GetPlanedQty() const;
	Price GetOpenStartPrice() const;

	TradeSystem::OrderId GetOpenOrderId() const;
	Qty GetOpenedQty() const;
	Price GetOpenPrice() const;
	Time GetOpenTime() const;

	Qty GetNotOpenedQty() const {
		Assert(GetOpenedQty() <= GetPlanedQty());
		return GetPlanedQty() - GetOpenedQty();
	}

	Qty GetActiveQty() const {
		Assert(GetOpenedQty() >= GetClosedQty());
		return GetOpenedQty() - GetClosedQty();
	}
	
	TradeSystem::OrderId GetCloseOrderId() const;
	void SetCloseStartPrice(Position::Price);
	Price GetCloseStartPrice() const;
	Price GetClosePrice() const;
	Qty GetClosedQty() const;
	Time GetCloseTime() const;

	Price GetCommission() const;

public:

	TradeSystem::OrderId OpenAtMarketPrice();
	TradeSystem::OrderId Open(Price);
	TradeSystem::OrderId OpenAtMarketPriceWithStopPrice(Price stopPrice);
	TradeSystem::OrderId OpenOrCancel(Price);

	TradeSystem::OrderId CloseAtMarketPrice();
	TradeSystem::OrderId Close(Price);
	TradeSystem::OrderId CloseAtMarketPriceWithStopPrice(Price stopPrice);
	TradeSystem::OrderId CloseOrCancel(Price);

//	void CancelPositionAtMarketPrice();
	void CancelAllOrders();

public:

	StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

	virtual Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot() = 0;
	virtual Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot() = 0;

protected:

	virtual TradeSystem::OrderId DoOpenAtMarketPrice() = 0;
	virtual TradeSystem::OrderId DoOpen(Price) = 0;
	virtual TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(Price stopPrice) = 0;
	virtual TradeSystem::OrderId DoOpenOrCancel(Price) = 0;

	virtual TradeSystem::OrderId DoCloseAtMarketPrice() = 0;
	virtual TradeSystem::OrderId DoClose(Price) = 0;
	virtual TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(Price stopPrice) = 0;
	virtual TradeSystem::OrderId DoCloseOrCancel(Price) = 0;

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

	boost::shared_ptr<Security> m_security;

	const Qty m_planedQty;

	const Price m_openStartPrice;
	DynamicData m_opened;

	Price m_closeStartPrice;
	DynamicData m_closed;

	volatile long m_state;

	CloseType m_closeType;

	bool m_isReported;

	const boost::shared_ptr<const Algo> m_algo;
	const boost::shared_ptr<AlgoPositionState> m_algoState;

	boost::function<void()> m_cancelMethod;

};

//////////////////////////////////////////////////////////////////////////

class LongPosition : public Position {

public:

	explicit LongPosition(
			boost::shared_ptr<Security> security,
			Qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
	virtual ~LongPosition();

public:

	virtual Type GetType() const;
	virtual const std::string & GetTypeStr() const;

public:
	
	virtual Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
	virtual Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

public:

	virtual TradeSystem::OrderId DoOpenAtMarketPrice();
	virtual TradeSystem::OrderId DoOpen(Price);
	virtual TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(Price stopPrice);
	virtual TradeSystem::OrderId DoOpenOrCancel(Price);

	virtual TradeSystem::OrderId DoCloseAtMarketPrice();
	virtual TradeSystem::OrderId DoClose(Price);
	virtual TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(Price stopPrice);
	virtual TradeSystem::OrderId DoCloseOrCancel(Price);

};

//////////////////////////////////////////////////////////////////////////

class ShortPosition : public Position {

public:

	explicit ShortPosition(
			boost::shared_ptr<Security> security,
			Qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
	virtual ~ShortPosition();

public:

	virtual Type GetType() const;
	virtual const std::string & GetTypeStr() const;

public:

	virtual Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
	virtual Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

public:

	virtual TradeSystem::OrderId DoOpenAtMarketPrice();
	virtual TradeSystem::OrderId DoOpen(Price);
	virtual TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(Price stopPrice);
	virtual TradeSystem::OrderId DoOpenOrCancel(Price);

	virtual TradeSystem::OrderId DoCloseAtMarketPrice();
	virtual TradeSystem::OrderId DoClose(Price);
	virtual TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(Price stopPrice);
	virtual TradeSystem::OrderId DoCloseOrCancel(Price);

};

//////////////////////////////////////////////////////////////////////////
