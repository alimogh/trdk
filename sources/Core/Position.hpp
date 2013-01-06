/**************************************************************************
 *   Created: 2012/07/09 17:32:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Types.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API Position
			: private boost::noncopyable,
			public boost::enable_shared_from_this<Trader::Position> {

	public:

		typedef void (StateUpdateSlotSignature)();
		typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
		typedef boost::signals2::connection StateUpdateConnection;

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

		class TRADER_CORE_API LogicError : public Trader::Lib::LogicError {
		public:
			explicit LogicError(const char *what) throw();
		};
		
		class TRADER_CORE_API AlreadyStartedError : public LogicError {
		public:
			AlreadyStartedError() throw();
		};
		class TRADER_CORE_API NotOpenedError : public LogicError {
		public:
			NotOpenedError() throw();
		};
		
		class TRADER_CORE_API AlreadyClosedError : public LogicError {
		public:
			AlreadyClosedError() throw();
		};

	public:

		explicit Position(
				Trader::Strategy &,
				Trader::Qty,
				Trader::ScaledPrice startPrice);
		virtual ~Position();

	public:

		virtual Type GetType() const = 0;
		virtual const std::string & GetTypeStr() const throw() = 0;

	public:

		const Trader::Security & GetSecurity() const throw();
		Trader::Security & GetSecurity() throw();

	public:

		CloseType GetCloseType() const throw();
		const std::string & GetCloseTypeStr() const;

		//! Has opened qty and hasn't active open-orders.
		bool IsOpened() const throw();
		//! Has opened qty and the same closed qty. Hasn't active close-orders.
		bool IsClosed() const throw();

		//! Started - one of open-orders sent.
		/** @sa	IsCompleted
		  */
		bool IsStarted() const throw();
		//! Started and now hasn't any orders and active qty.
		/** @sa	IsStarted
		  */
		bool IsCompleted() const throw();

		//! Open operation started, but error occurred at opening or closing.
		bool IsError() const throw();
		//! All orders canceled, position will be closed or already closed.
		bool IsCanceled() const throw();

		bool HasActiveOrders() const throw();
		bool HasActiveOpenOrders() const throw();
		bool HasActiveCloseOrders() const throw();

	public:

		Trader::Qty GetPlanedQty() const;
		Trader::ScaledPrice GetOpenStartPrice() const;

		Trader::OrderId GetOpenOrderId() const throw();
		Trader::Qty GetOpenedQty() const throw();
		Trader::ScaledPrice GetOpenPrice() const;
		Time GetOpenTime() const;

		Trader::Qty GetNotOpenedQty() const;
		Trader::Qty GetActiveQty() const throw();

		Trader::OrderId GetCloseOrderId() const throw();
		void SetCloseStartPrice(Trader::ScaledPrice);
		Trader::ScaledPrice GetCloseStartPrice() const;
		Trader::ScaledPrice GetClosePrice() const;
		Trader::Qty GetClosedQty() const throw();
		Time GetCloseTime() const;

		Trader::ScaledPrice GetCommission() const;

	public:

		Trader::OrderId OpenAtMarketPrice();
		Trader::OrderId Open(Trader::ScaledPrice);
		Trader::OrderId OpenAtMarketPriceWithStopPrice(
					Trader::ScaledPrice stopPrice);
		Trader::OrderId OpenOrCancel(Trader::ScaledPrice);

		Trader::OrderId CloseAtMarketPrice(CloseType);
		Trader::OrderId Close(CloseType, Trader::ScaledPrice);
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
		virtual Trader::OrderId DoOpen(Trader::Qty, Trader::ScaledPrice) = 0;
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice)
				= 0;
		virtual Trader::OrderId DoOpenOrCancel(
					Trader::Qty,
					Trader::ScaledPrice)
				= 0;

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty) = 0;
		virtual Trader::OrderId DoClose(Trader::Qty, Trader::ScaledPrice) = 0;
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice)
				= 0;
		virtual Trader::OrderId DoCloseOrCancel(
					Trader::Qty,
					Trader::ScaledPrice)
				= 0;

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

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API LongPosition : public Position {

	public:

		explicit LongPosition(
				Trader::Strategy &,
				Trader::Qty,
				Trader::ScaledPrice startPrice);
		virtual ~LongPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot
		GetSellOrderStatusUpdateSlot();
		
		virtual Trader::Security::OrderStatusUpdateSlot
		GetBuyOrderStatusUpdateSlot();

	protected:

		virtual Trader::OrderId DoOpenAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoOpen(Trader::Qty, Trader::ScaledPrice);
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoOpenOrCancel(
					Trader::Qty,
					Trader::ScaledPrice);

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoClose(Trader::Qty, Trader::ScaledPrice);
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoCloseOrCancel(
					Trader::Qty,
					Trader::ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API ShortPosition : public Position {

	public:

		explicit ShortPosition(
				Trader::Strategy &,
				Trader::Qty,
				Trader::ScaledPrice startPrice);
		virtual ~ShortPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	public:

		virtual Trader::Security::OrderStatusUpdateSlot
		GetSellOrderStatusUpdateSlot();
		
		virtual Trader::Security::OrderStatusUpdateSlot
		GetBuyOrderStatusUpdateSlot();

	protected:

		virtual Trader::OrderId DoOpenAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoOpen(Trader::Qty, Trader::ScaledPrice);
		virtual Trader::OrderId DoOpenAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoOpenOrCancel(
					Trader::Qty,
					Trader::ScaledPrice);

		virtual Trader::OrderId DoCloseAtMarketPrice(Trader::Qty);
		virtual Trader::OrderId DoClose(Trader::Qty, Trader::ScaledPrice);
		virtual Trader::OrderId DoCloseAtMarketPriceWithStopPrice(
					Trader::Qty,
					Trader::ScaledPrice stopPrice);
		virtual Trader::OrderId DoCloseOrCancel(
					Trader::Qty,
					Trader::ScaledPrice);

	};

	//////////////////////////////////////////////////////////////////////////

}
