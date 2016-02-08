/**************************************************************************
 *   Created: 2012/07/09 17:32:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class LongPosition;
	class ShortPosition;

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API Position
			: private boost::noncopyable,
			public boost::enable_shared_from_this<trdk::Position> {

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
			//! Closed by request, not by logic or error.
			CLOSE_TYPE_REQUEST,
			//! Closed by engine stop.
			CLOSE_TYPE_ENGINE_STOP,
			CLOSE_TYPE_OPEN_FAILED,
			CLOSE_TYPE_SYSTEM_ERROR,
			numberOfCloseTypes
		};

		class TRDK_CORE_API LogicError : public trdk::Lib::LogicError {
		public:
			explicit LogicError(const char *what) throw();
		};
		
		class TRDK_CORE_API AlreadyStartedError : public LogicError {
		public:
			AlreadyStartedError() throw();
		};
		class TRDK_CORE_API NotOpenedError : public LogicError {
		public:
			NotOpenedError() throw();
		};
		
		class TRDK_CORE_API AlreadyClosedError : public LogicError {
		public:
			AlreadyClosedError() throw();
		};

	public:

		explicit Position(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::TradeSystem &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		explicit Position(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::Position &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		virtual ~Position();

	public:

		const boost::uuids::uuid & GetId() const;

		virtual Type GetType() const = 0;
		virtual const std::string & GetTypeStr() const throw() = 0;

	public:

		const trdk::TradeSystem & GetTradeSystem() const;
		trdk::TradeSystem & GetTradeSystem();

		const trdk::Strategy & GetStrategy() const throw();
		trdk::Strategy & GetStrategy() throw();

		const trdk::Security & GetSecurity() const throw();
		trdk::Security & GetSecurity() throw();

		const trdk::Lib::Currency & GetCurrency() const;

		const Lib::TimeMeasurement::Milestones & GetTimeMeasurement();

	public:

		CloseType GetCloseType() const throw();
		const std::string & GetCloseTypeStr() const;

		//! Has opened qty and hasn't active open-orders.
		/** @sa	IsClosed
		  */
		bool IsOpened() const throw();
		//! Closed.
		/** First was opened, then closed, hasn't active quantity and active
		  * orders.
		  * @sa	IsOpened
		  */
		bool IsClosed() const throw();

		//! Started.
		/** True if at least one of open-orders was sent.
		/** @sa	IsCompleted
		  */
		bool IsStarted() const throw();
		//! Started and now hasn't any orders and active qty or market as
		//! completed.
		/** @sa	IsStarted
		  * @sa MarkAsCompleted
		  */
		bool IsCompleted() const throw();
		//! Marks position as completed without real closing.
		/** Call not thread-safe! Must be called only from event-methods, or if
		  * strategy locked by GetMutex().
		  * @sa IsCompleted
		  */
		void MarkAsCompleted();

		//! Open operation started, but error occurred at opening or closing.
		bool IsError() const throw();
		//! Open operation started, but temporary error occurred at opening
		//! or closing.
		bool IsInactive() const throw();
		void ResetInactive();
		//! All orders canceled, position will be closed or already closed.
		bool IsCanceled() const throw();

		bool HasActiveOrders() const throw();
		bool HasActiveOpenOrders() const throw();
		bool HasActiveCloseOrders() const throw();

	public:

		trdk::Qty GetPlanedQty() const;
		const trdk::ScaledPrice & GetOpenStartPrice() const;

		trdk::OrderId GetOpenOrderId() const throw();
		void SetOpenedQty(const trdk::Qty &) const throw();
		trdk::Qty GetOpenedQty() const throw();
		trdk::ScaledPrice GetOpenPrice() const;
		double GetOpenedVolume() const;
		Time GetOpenTime() const;

		trdk::Qty GetNotOpenedQty() const;
		trdk::Qty GetActiveQty() const throw();

		trdk::OrderId GetCloseOrderId() const throw();
		void SetCloseStartPrice(const trdk::ScaledPrice &);
		trdk::ScaledPrice GetCloseStartPrice() const;
		trdk::ScaledPrice GetClosePrice() const;
		trdk::Qty GetClosedQty() const throw();
		Time GetCloseTime() const;

		trdk::ScaledPrice GetCommission() const;

	public:

		//! Restores position in open-state.
		/** Just creates position in open state at current strategy, doesn't
		  * make any trading actions.
		  * @param openOrderId	User-defined ID for open-order, doesn't affect
		  *						the engine logic.
		  */
		void RestoreOpenState(trdk::OrderId openOrderId = 0);

		trdk::OrderId OpenAtMarketPrice();
		trdk::OrderId OpenAtMarketPrice(const trdk::OrderParams &);
		trdk::OrderId Open(const trdk::ScaledPrice &);
		trdk::OrderId Open(
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		trdk::OrderId OpenAtMarketPriceWithStopPrice(
				const trdk::ScaledPrice &stopPrice);
		trdk::OrderId OpenAtMarketPriceWithStopPrice(
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		trdk::OrderId OpenImmediatelyOrCancel(
				const trdk::ScaledPrice &);
		trdk::OrderId OpenImmediatelyOrCancel(
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		trdk::OrderId OpenAtMarketPriceImmediatelyOrCancel();
		trdk::OrderId OpenAtMarketPriceImmediatelyOrCancel(
				const trdk::OrderParams &);

		trdk::OrderId CloseAtMarketPrice(
				const CloseType &);
		trdk::OrderId CloseAtMarketPrice(
				const CloseType &,
				const trdk::OrderParams &);
		trdk::OrderId Close(
				const CloseType &,
				const trdk::ScaledPrice &);
		trdk::OrderId Close(
				const CloseType &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		trdk::OrderId CloseAtMarketPriceWithStopPrice(
				const CloseType &,
				const trdk::ScaledPrice &stopPrice);
		trdk::OrderId CloseAtMarketPriceWithStopPrice(
				const CloseType &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		trdk::OrderId CloseImmediatelyOrCancel(
				const CloseType &,
				const trdk::ScaledPrice &);
		trdk::OrderId CloseImmediatelyOrCancel(
				const CloseType &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		trdk::OrderId CloseAtMarketPriceImmediatelyOrCancel(const CloseType &);
		trdk::OrderId CloseAtMarketPriceImmediatelyOrCancel(
				const CloseType &,
				const trdk::OrderParams &);

		bool CancelAtMarketPrice(const CloseType &);
		bool CancelAtMarketPrice(
				const CloseType &,
				const trdk::OrderParams &);
		bool CancelAllOrders();

	public:

		StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

	protected:

		virtual trdk::OrderId DoOpenAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoOpen(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoOpenAtMarketPriceWithStopPrice(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoOpenImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoOpenAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &)
			= 0;

		virtual trdk::OrderId DoCloseAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoClose(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoCloseAtMarketPriceWithStopPrice(
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoCloseImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &)
			= 0;
		virtual trdk::OrderId DoCloseAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &)
			= 0;

	protected:

		void UpdateOpening(
				const trdk::OrderId &,
				const std::string &tradeSystemOrderId,
				const trdk::OrderStatus &,
				const trdk::Qty &remainingQty,
				const trdk::TradeSystem::TradeInfo *);
		void UpdateClosing(
				const trdk::OrderId &,
				const std::string &tradeSystemOrderId,
				const trdk::OrderStatus &,
				const trdk::Qty &remainingQty,
				const trdk::TradeSystem::TradeInfo *);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API LongPosition : virtual public Position {

	public:

		explicit LongPosition(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::TradeSystem &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &);
		explicit LongPosition(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::ShortPosition &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &);
		virtual ~LongPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	protected:

		virtual trdk::OrderId DoOpenAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpen(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenAtMarketPriceWithStopPrice(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &);

		virtual trdk::OrderId DoCloseAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoClose(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseAtMarketPriceWithStopPrice(
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &);

	};

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API ShortPosition : virtual public Position {

	public:

		explicit ShortPosition(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::TradeSystem &,
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &);
		explicit ShortPosition(
				trdk::Strategy &,
				const boost::uuids::uuid &operationId,
				int64_t subOperationId,
				trdk::LongPosition &,
				const trdk::Qty &,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &);
		virtual ~ShortPosition();

	public:

		virtual Type GetType() const;
		virtual const std::string & GetTypeStr() const throw();

	protected:

		virtual trdk::OrderId DoOpenAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpen(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenAtMarketPriceWithStopPrice(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoOpenAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &);

		virtual trdk::OrderId DoCloseAtMarketPrice(
				const trdk::Qty &qty,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoClose(
				const trdk::Qty &qty,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseAtMarketPriceWithStopPrice(
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &);
		virtual trdk::OrderId DoCloseAtMarketPriceImmediatelyOrCancel(
				const trdk::Qty &,
				const trdk::OrderParams &);

	};

	//////////////////////////////////////////////////////////////////////////

}
