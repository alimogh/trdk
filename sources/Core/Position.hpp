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

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API StrategyPositionState {
	public:
		StrategyPositionState();
		virtual ~StrategyPositionState();
	};

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

		typedef boost::posix_time::ptime Time;

		enum Type {
			TYPE_LONG,
			TYPE_SHORT,
			numberOfTypes
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

		typedef boost::signals2::signal<StateUpdateSlotSignature>
			StateUpdateSignal;

		struct DynamicData {

			volatile Trader::OrderId lastOrderId;
			Time time;
			struct Price {
				volatile long long total;
				volatile long count;
				Price();
			} price;
			volatile Trader::Qty qty;
			volatile Trader::ScaledPrice comission;

			volatile bool hasOrder;

			DynamicData();

		};

	public:

		//! Creates position with dynamic planed qty
		explicit Position(
				boost::shared_ptr<Trader::Security>,
				const std::string &tag);
		//! Creates position with fixed planed qty and open price
		explicit Position(
				boost::shared_ptr<Trader::Security>,
				Trader::Qty,
				Trader::ScaledPrice startPrice,
				const std::string &tag,
				boost::shared_ptr<StrategyPositionState>
					= boost::shared_ptr<StrategyPositionState>());
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

		void SetStrategyState(
					boost::shared_ptr<StrategyPositionState> strategyState) {
			m_strategyState = strategyState;
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
			return
				!HasActiveCloseOrders()
				&& GetOpenedQty() > 0
				&& GetActiveQty() == 0;
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

		Trader::Qty GetPlanedQty() const;
		Trader::ScaledPrice GetOpenStartPrice() const;

		Trader::OrderId GetOpenOrderId() const throw() {
			return m_opened.lastOrderId;
		}
		Trader::Qty GetOpenedQty() const throw() {
			return m_opened.qty;
		}
		Trader::ScaledPrice GetOpenPrice() const;
		Time GetOpenTime() const;

		Trader::Qty GetNotOpenedQty() const {
			Assert(GetOpenedQty() <= GetPlanedQty());
			return GetPlanedQty() - GetOpenedQty();
		}

		Trader::Qty GetActiveQty() const throw() {
			Assert(GetOpenedQty() >= GetClosedQty());
			return GetOpenedQty() - GetClosedQty();
		}

		Trader::OrderId GetCloseOrderId() const throw() {
			return m_closed.lastOrderId;
		}
		void SetCloseStartPrice(Trader::ScaledPrice);
		Trader::ScaledPrice GetCloseStartPrice() const;
		Trader::ScaledPrice GetClosePrice() const;
		Trader::Qty GetClosedQty() const throw() {
			return m_closed.qty;
		}
		Time GetCloseTime() const;

		Trader::ScaledPrice GetCommission() const;

	public:

		void IncreasePlanedQty(Trader::Qty) throw();

	public:

		Trader::OrderId OpenAtMarketPrice();
		Trader::OrderId Open(Trader::ScaledPrice);
		Trader::OrderId OpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		Trader::OrderId OpenOrCancel(Trader::ScaledPrice);

		Trader::OrderId CloseAtMarketPrice(CloseType);
		Trader::OrderId CloseAtMarketPrice(CloseType, Trader::Qty);
		Trader::OrderId Close(CloseType, Trader::ScaledPrice);
		Trader::OrderId Close(CloseType, Trader::ScaledPrice, Trader::Qty);
		Trader::OrderId CloseAtMarketPriceWithStopPrice(
					CloseType,
					Trader::ScaledPrice stopPrice);
		Trader::OrderId CloseOrCancel(CloseType, Trader::ScaledPrice);

		bool CancelAtMarketPrice(CloseType);
		bool CancelAllOrders();

	public:

		StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

		virtual Trader::Security::OrderStatusUpdateSlot
		GetSellOrderStatusUpdateSlot() = 0;
		
		virtual Trader::Security::OrderStatusUpdateSlot
		GetBuyOrderStatusUpdateSlot() = 0;

	protected:

		virtual Trader::OrderId DoOpenAtMarketPrice(Trader::Qty) = 0;
		virtual Trader::OrderId DoOpen(Trader::ScaledPrice, Trader::Qty) = 0;
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice)
				= 0;
		virtual Trader::OrderId DoOpenOrCancel(Trader::ScaledPrice) = 0;

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty) = 0;
		virtual Trader::OrderId DoClose(Trader::ScaledPrice, Trader::Qty) = 0;
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice)
				= 0;
		virtual Trader::OrderId DoCloseOrCancel(Trader::ScaledPrice) = 0;

		bool DoCancelAllOrders();

	protected:

		void UpdateOpening(
					Trader::OrderId,
					Trader::TradeSystem::OrderStatus,
					Trader::Qty filled,
					Trader::Qty remaining,
					double avgPrice,
					double lastPrice);
		void UpdateClosing(
					Trader::OrderId,
					Trader::TradeSystem::OrderStatus,
					Trader::Qty filled,
					Trader::Qty remaining,
					double avgPrice,
					double lastPrice);

		void DecreasePlanedQty(Trader::Qty) throw();

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

		const Trader::ScaledPrice m_openStartPrice;
		DynamicData m_opened;

		Trader::ScaledPrice m_closeStartPrice;
		DynamicData m_closed;

		volatile long m_closeType;

		bool m_isReported;

		volatile long m_isError;

		volatile long m_isCanceled;
		boost::function<void()> m_cancelMethod;

		const std::string m_tag;
		boost::shared_ptr<StrategyPositionState> m_strategyState;

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API LongPosition : public Position {

	public:

		//! Creates position with dynamic planed qty
		explicit LongPosition(
				boost::shared_ptr<Trader::Security>,
				const std::string &tag);
		//! Creates position with fixed planed qty and open price
		explicit LongPosition(
				boost::shared_ptr<Trader::Security>,
				Trader::Qty,
				Trader::ScaledPrice startPrice,
				const std::string &tag,
				boost::shared_ptr<StrategyPositionState>
					= boost::shared_ptr<StrategyPositionState>());
		virtual ~LongPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot
		GetSellOrderStatusUpdateSlot();
		
		virtual Trader::Security::OrderStatusUpdateSlot
		GetBuyOrderStatusUpdateSlot();

	public:

		virtual Trader::OrderId DoOpenAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoOpen(Trader::ScaledPrice, Trader::Qty);
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoOpenOrCancel(Trader::ScaledPrice);

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoClose(Trader::ScaledPrice, Trader::Qty);
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoCloseOrCancel(Trader::ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API ShortPosition : public Position {

	public:

		//! Creates position with dynamic planed qty
		explicit ShortPosition(
				boost::shared_ptr<Trader::Security>,
				const std::string &tag);
		//! Creates position with fixed planed qty and open price
		explicit ShortPosition(
				boost::shared_ptr<Trader::Security>,
				Trader::Qty,
				Trader::ScaledPrice startPrice,
				const std::string &tag,
				boost::shared_ptr<StrategyPositionState>
					= boost::shared_ptr<StrategyPositionState>());
		virtual ~ShortPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot
		GetSellOrderStatusUpdateSlot();
		
		virtual Trader::Security::OrderStatusUpdateSlot
		GetBuyOrderStatusUpdateSlot();

	public:

		virtual Trader::OrderId DoOpenAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoOpen(Trader::ScaledPrice, Trader::Qty);
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoOpenOrCancel(Trader::ScaledPrice);

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoClose(Trader::ScaledPrice, Trader::Qty);
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoCloseOrCancel(Trader::ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

}
