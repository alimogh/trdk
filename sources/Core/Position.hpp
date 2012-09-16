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
#include "Api.h"

class Algo;
class AlgoPositionState;

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API Position
			: private boost::noncopyable,
			public boost::enable_shared_from_this<Position> {

	public:

		typedef void (StateUpdateSlotSignature)();
		typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
		typedef SignalConnection<
				StateUpdateSlot,
				boost::signals2::connection>
			StateUpdateConnection;

		typedef Trader::Security::ScaledPrice ScaledPrice;
		typedef Trader::Security::Qty Qty;
		typedef boost::posix_time::ptime Time;

		enum Type {
			TYPE_LONG,
			TYPE_SHORT
		};

		enum CloseType {
			CLOSE_TYPE_NONE,
			CLOSE_TYPE_TAKE_PROFIT,
			CLOSE_TYPE_STOP_LOSS,
			CLOSE_TYPE_TIMEOUT,
			CLOSE_TYPE_SCHEDULE,
			CLOSE_TYPE_ENGINE_STOP
		};

		typedef boost::int64_t AlgoFlag;

	private:

		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;

		typedef boost::signals2::signal<StateUpdateSlotSignature> StateUpdateSignal;

		struct DynamicData {

			volatile long long orderId;
			Time time;
			volatile long long price;
			volatile long qty;
			volatile long long comission;
		
			volatile long hasOrder;

			DynamicData();

		};

	public:

		explicit Position(
				boost::shared_ptr<Trader::Security> security,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Algo> algo,
				boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
		virtual ~Position();

	public:

		virtual Type GetType() const = 0;
		virtual const std::string & GetTypeStr() const throw() = 0;

	public:

		const Trader::Security & GetSecurity() const throw() {
			return const_cast<Position *>(this)->GetSecurity();
		}
		Trader::Security & GetSecurity() throw() {
			return *m_security;
		}

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

		CloseType GetCloseType() const throw() {
			return CloseType(m_closeType);
		}
		const std::string & GetCloseTypeStr() const;

		bool IsReported() const;
		void MarkAsReported();

		bool IsOpened() const throw() {
			return !HasActiveOpenOrders() && GetOpenedQty() > 0;
		}
		bool IsClosed() const throw() {
			return !HasActiveCloseOrders() && GetOpenedQty() > 0 && GetActiveQty() == 0;
		}

		bool IsCompleted() const throw() {
			return !HasActiveOrders() && GetActiveQty() == 0;
		}

		bool IsError() const throw() {
			return m_isError ? true : false;
		}
		bool IsCanceled() const throw() {
			return m_isCanceled ? true : false;
		}

		bool HasActiveOrders() const throw() {
			return HasActiveCloseOrders() || HasActiveOpenOrders();
		}
		bool HasActiveOpenOrders() const throw() {
			return m_opened.hasOrder ? true : false;
		}
		bool HasActiveCloseOrders() const throw() {
			return m_closed.hasOrder ? true : false;
		}

	public:

		Qty GetPlanedQty() const;
		ScaledPrice GetOpenStartPrice() const;

		Trader::TradeSystem::OrderId GetOpenOrderId() const throw() {
			return Trader::TradeSystem::OrderId(m_opened.orderId);
		}
		Qty GetOpenedQty() const throw() {
			return m_opened.qty;
		}
		ScaledPrice GetOpenPrice() const;
		Time GetOpenTime() const;

		Qty GetNotOpenedQty() const {
			Assert(GetOpenedQty() <= GetPlanedQty());
			return GetPlanedQty() - GetOpenedQty();
		}

		Qty GetActiveQty() const {
			Assert(GetOpenedQty() >= GetClosedQty());
			return GetOpenedQty() - GetClosedQty();
		}
	
		Trader::TradeSystem::OrderId GetCloseOrderId() const throw() {
			return Trader::TradeSystem::OrderId(m_closed.orderId);
		}
		void SetCloseStartPrice(Position::ScaledPrice);
		ScaledPrice GetCloseStartPrice() const;
		ScaledPrice GetClosePrice() const;
		Qty GetClosedQty() const throw() {
			return m_closed.qty;
		}
		Time GetCloseTime() const;

		ScaledPrice GetCommission() const;

	public:

		Trader::TradeSystem::OrderId OpenAtMarketPrice();
		Trader::TradeSystem::OrderId Open(ScaledPrice);
		Trader::TradeSystem::OrderId OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		Trader::TradeSystem::OrderId OpenOrCancel(ScaledPrice);

		Trader::TradeSystem::OrderId CloseAtMarketPrice(CloseType);
		Trader::TradeSystem::OrderId Close(CloseType, ScaledPrice);
		Trader::TradeSystem::OrderId CloseAtMarketPriceWithStopPrice(CloseType, ScaledPrice stopPrice);
		Trader::TradeSystem::OrderId CloseOrCancel(CloseType, ScaledPrice);

		bool CancelAtMarketPrice(CloseType);
		bool CancelAllOrders();

	public:

		StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot() = 0;
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot() = 0;

	protected:

		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPrice() = 0;
		virtual Trader::TradeSystem::OrderId DoOpen(ScaledPrice) = 0;
		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) = 0;
		virtual Trader::TradeSystem::OrderId DoOpenOrCancel(ScaledPrice) = 0;

		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPrice() = 0;
		virtual Trader::TradeSystem::OrderId DoClose(ScaledPrice) = 0;
		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice) = 0;
		virtual Trader::TradeSystem::OrderId DoCloseOrCancel(ScaledPrice) = 0;

		bool DoCancelAllOrders();

	protected:

		void UpdateOpening(
					Trader::TradeSystem::OrderId,
					Trader::TradeSystem::OrderStatus,
					Qty filled,
					Qty remaining,
					double avgPrice,
					double lastPrice);
		void UpdateClosing(
					Trader::TradeSystem::OrderId,
					Trader::TradeSystem::OrderStatus,
					Qty filled,
					Qty remaining,
					double avgPrice,
					double lastPrice);

	private:

		bool CancelIfSet() throw();

		void ReportOpeningUpdate(
					const char *eventDesc,
					Trader::TradeSystem::OrderStatus)
				const
				throw();
		void ReportClosingUpdate(
					const char *eventDesc,
					Trader::TradeSystem::OrderStatus)
				const
				throw();

	private:

		mutable Mutex m_mutex;

		mutable StateUpdateSignal m_stateUpdateSignal;

		boost::shared_ptr<Trader::Security> m_security;

		const Qty m_planedQty;

		const ScaledPrice m_openStartPrice;
		DynamicData m_opened;

		ScaledPrice m_closeStartPrice;
		DynamicData m_closed;

		volatile long m_closeType;

		bool m_isReported;
	
		volatile long m_isError;
	
		volatile long m_isCanceled;
		boost::function<void()> m_cancelMethod;

		const boost::shared_ptr<const Algo> m_algo;
		const boost::shared_ptr<AlgoPositionState> m_algoState;

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API LongPosition : public Position {

	public:

		explicit LongPosition(
				boost::shared_ptr<Trader::Security> security,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Algo> algo,
				boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
		virtual ~LongPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:
	
		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

	public:

		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPrice();
		virtual Trader::TradeSystem::OrderId DoOpen(ScaledPrice);
		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual Trader::TradeSystem::OrderId DoOpenOrCancel(ScaledPrice);

		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPrice();
		virtual Trader::TradeSystem::OrderId DoClose(ScaledPrice);
		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual Trader::TradeSystem::OrderId DoCloseOrCancel(ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API ShortPosition : public Position {

	public:

		explicit ShortPosition(
				boost::shared_ptr<Trader::Security> security,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Algo> algo,
				boost::shared_ptr<AlgoPositionState> state = boost::shared_ptr<AlgoPositionState>());
		virtual ~ShortPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

	public:

		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPrice();
		virtual Trader::TradeSystem::OrderId DoOpen(ScaledPrice);
		virtual Trader::TradeSystem::OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual Trader::TradeSystem::OrderId DoOpenOrCancel(ScaledPrice);

		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPrice();
		virtual Trader::TradeSystem::OrderId DoClose(ScaledPrice);
		virtual Trader::TradeSystem::OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual Trader::TradeSystem::OrderId DoCloseOrCancel(ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

}
