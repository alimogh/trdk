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

class StrategyPositionState;

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

		typedef Trader::TradeSystem::OrderId OrderId;
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

	private:

		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;

		typedef boost::signals2::signal<StateUpdateSlotSignature> StateUpdateSignal;

		struct DynamicData {

			volatile OrderId lastOrderId;
			Time time;
			struct Price {
				volatile long long total;
				volatile long count;
				Price();
			} price;
			volatile Qty qty;
			volatile ScaledPrice comission;

			volatile bool hasOrder;

			DynamicData();

		};

	public:

		//! Creates position with dynamic planed qty
		explicit Position(boost::shared_ptr<Trader::Security>);
		//! Creates position with fixed planed qty and open price
		explicit Position(
				boost::shared_ptr<Trader::Security>,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Strategy>,
				boost::shared_ptr<StrategyPositionState> = boost::shared_ptr<StrategyPositionState>());
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

		const Strategy & GetStrategy() const {
			return *m_strategy;
		}

		template<typename StrategyState>
		bool IsStrategyStateSet() const {
			return
				m_strategyState
				&& dynamic_cast<const StrategyState *>(m_strategyState.get());
		}

		template<typename StrategyState>
		StrategyState & GetStrategyState() {
			Assert(IsStrategyStateSet<StrategyState>());
			return *boost::polymorphic_downcast<StrategyState *>(
				m_strategyState.get());
		}
		template<typename StrategyState>
		const StrategyState & GetStrategyState() const {
			return const_cast<Position *>(this)
				->GetStrategyState<StrategyState>();
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

		OrderId GetOpenOrderId() const throw() {
			return m_opened.lastOrderId;
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

		Qty GetActiveQty() const throw() {
			Assert(GetOpenedQty() >= GetClosedQty());
			return GetOpenedQty() - GetClosedQty();
		}

		OrderId GetCloseOrderId() const throw() {
			return m_closed.lastOrderId;
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

		void IncreasePlanedQty(Qty) throw();

	public:

		OrderId OpenAtMarketPrice();
		OrderId Open(ScaledPrice);
		OrderId OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		OrderId OpenOrCancel(ScaledPrice);

		OrderId CloseAtMarketPrice(CloseType);
		OrderId CloseAtMarketPrice(CloseType, Qty);
		OrderId Close(CloseType, ScaledPrice);
		OrderId Close(CloseType, ScaledPrice, Qty);
		OrderId CloseAtMarketPriceWithStopPrice(CloseType, ScaledPrice stopPrice);
		OrderId CloseOrCancel(CloseType, ScaledPrice);

		bool CancelAtMarketPrice(CloseType);
		bool CancelAllOrders();

	public:

		StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot() = 0;
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot() = 0;

	protected:

		virtual OrderId DoOpenAtMarketPrice(Qty) = 0;
		virtual OrderId DoOpen(ScaledPrice, Qty) = 0;
		virtual OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) = 0;
		virtual OrderId DoOpenOrCancel(ScaledPrice) = 0;

		virtual OrderId DoCloseAtMarketPrice(Qty) = 0;
		virtual OrderId DoClose(ScaledPrice, Qty) = 0;
		virtual OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice) = 0;
		virtual OrderId DoCloseOrCancel(ScaledPrice) = 0;

		bool DoCancelAllOrders();

	protected:

		void UpdateOpening(
					OrderId,
					Trader::TradeSystem::OrderStatus,
					Qty filled,
					Qty remaining,
					double avgPrice,
					double lastPrice);
		void UpdateClosing(
					OrderId,
					Trader::TradeSystem::OrderStatus,
					Qty filled,
					Qty remaining,
					double avgPrice,
					double lastPrice);

		void DecreasePlanedQty(Qty) throw();

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

		const bool m_isPlanedQtyDynamic;
		volatile long m_planedQty;

		const ScaledPrice m_openStartPrice;
		DynamicData m_opened;

		ScaledPrice m_closeStartPrice;
		DynamicData m_closed;

		volatile long m_closeType;

		bool m_isReported;

		volatile long m_isError;

		volatile long m_isCanceled;
		boost::function<void()> m_cancelMethod;

		const boost::shared_ptr<const Strategy> m_strategy;
		const boost::shared_ptr<StrategyPositionState> m_strategyState;

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API LongPosition : public Position {

	public:

		//! Creates position with dynamic planed qty
		explicit LongPosition(boost::shared_ptr<Trader::Security>);
		//! Creates position with fixed planed qty and open price
		explicit LongPosition(
				boost::shared_ptr<Trader::Security>,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Strategy> = boost::shared_ptr<const Strategy>(),
				boost::shared_ptr<StrategyPositionState> = boost::shared_ptr<StrategyPositionState>());
		virtual ~LongPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

	public:

		virtual OrderId DoOpenAtMarketPrice(Qty);
		virtual OrderId DoOpen(ScaledPrice, Qty);
		virtual OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual OrderId DoOpenOrCancel(ScaledPrice);

		virtual OrderId DoCloseAtMarketPrice(Qty);
		virtual OrderId DoClose(ScaledPrice, Qty);
		virtual OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual OrderId DoCloseOrCancel(ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API ShortPosition : public Position {

	public:

		//! Creates position with dynamic planed qty
		explicit ShortPosition(boost::shared_ptr<Trader::Security>);
		//! Creates position with fixed planed qty and open price
		explicit ShortPosition(
				boost::shared_ptr<Trader::Security>,
				Qty,
				ScaledPrice startPrice,
				boost::shared_ptr<const Strategy> = boost::shared_ptr<const Strategy>(),
				boost::shared_ptr<StrategyPositionState> = boost::shared_ptr<StrategyPositionState>());
		virtual ~ShortPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot GetSellOrderStatusUpdateSlot();
		virtual Trader::Security::OrderStatusUpdateSlot GetBuyOrderStatusUpdateSlot();

	public:

		virtual OrderId DoOpenAtMarketPrice(Qty);
		virtual OrderId DoOpen(ScaledPrice, Qty);
		virtual OrderId DoOpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual OrderId DoOpenOrCancel(ScaledPrice);

		virtual OrderId DoCloseAtMarketPrice(Qty);
		virtual OrderId DoClose(ScaledPrice, Qty);
		virtual OrderId DoCloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		virtual OrderId DoCloseOrCancel(ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

}
