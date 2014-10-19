/**************************************************************************
 *   Created: 2014/08/12 22:17:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "CurrenexSession.hpp"
#include "Core/TradeSystem.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	template<Lib::Concurrency::Profile profile>
	struct CurrenexTradingConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::shared_mutex OrderMutex;
		typedef boost::shared_lock<OrderMutex> OrderReadLock;
		typedef boost::unique_lock<OrderMutex> OrderWriteLock;
		typedef boost::mutex SendMutex;
		typedef boost::unique_lock<SendMutex> SendLock;
		typedef boost::condition_variable SendCondition;
	};
	template<>
	struct CurrenexTradingConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		//! @todo TRDK-168
		typedef Lib::Concurrency::SpinMutex OrderMutex;
		typedef OrderMutex::ScopedLock OrderReadLock;
		typedef OrderMutex::ScopedLock OrderWriteLock;
		typedef Lib::Concurrency::SpinMutex SendMutex;
		typedef SendMutex::ScopedLock SendLock;
		typedef Lib::Concurrency::SpinCondition SendCondition;
	};
	typedef CurrenexTradingConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
		CurrenexTradingConcurrencyPolicy;

	//! FIX trade connection with OnixS C++ FIX Engine.
	class CurrenexTrading
			: public trdk::TradeSystem,
			public OnixS::FIX::ISessionListener {

	private:

		struct Order {
			//! Order must be removed from storage.
			/** Synchronization doesn't require - it provides FIX engine.
			  */
			bool isRemoved;
			OrderId id;
			OrderStatusUpdateSlot callback;
			Lib::TimeMeasurement::Milestones timeMeasurement;
		};

		struct OrderToSend {
			OrderId id;
			const trdk::Security *security;
			trdk::Lib::Currency currency;
			trdk::Qty qty;
			bool isSell;
			Lib::TimeMeasurement::Milestones timeMeasurement;
		};

		typedef CurrenexTradingConcurrencyPolicy::OrderMutex OrdersMutex;
		typedef CurrenexTradingConcurrencyPolicy::OrderReadLock OrdersReadLock;
		typedef CurrenexTradingConcurrencyPolicy::OrderWriteLock
			OrdersWriteLock;

		typedef CurrenexTradingConcurrencyPolicy::SendMutex SendMutex;
		typedef CurrenexTradingConcurrencyPolicy::SendLock SendLock;
		typedef CurrenexTradingConcurrencyPolicy::SendCondition SendCondition;

	public:

		explicit CurrenexTrading(
					Context &,
					const std::string &tag,
					const Lib::IniSectionRef &);
		virtual ~CurrenexTrading();

	public:

		virtual bool IsConnected() const;

	public:

		virtual OrderId SellAtMarketPrice(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					trdk::ScaledPrice,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					trdk::ScaledPrice stopPrice,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId SellImmediatelyOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					const trdk::Qty &,
					const trdk::ScaledPrice &,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceImmediatelyOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					const trdk::Qty &,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					trdk::ScaledPrice,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty qty,
					trdk::ScaledPrice stopPrice,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId BuyImmediatelyOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					const trdk::Qty &,
					const trdk::ScaledPrice &,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceImmediatelyOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					const trdk::Qty &,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(trdk::Security &);

	public:

		virtual void onInboundApplicationMsg(
					OnixS::FIX::Message &,
					OnixS::FIX::Session *);
		virtual void onStateChange(
					OnixS::FIX::SessionState::Enum newState,
					OnixS::FIX::SessionState::Enum prevState,
					OnixS::FIX::Session *);
		virtual void onError (
					OnixS::FIX::ErrorReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *session);
		virtual void onWarning (
					OnixS::FIX::WarningReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *);
	
	protected:

		virtual void CreateConnection(const trdk::Lib::IniSectionRef &);

	private:

		//! Takes next free order ID.
		OrderId TakeOrderId(
					const OrderStatusUpdateSlot &,
					const Lib::TimeMeasurement::Milestones &);
		//! Deletes unsent order (at error).
		void DeleteErrorOrder(const OrderId &) throw();

		Order * FindOrder(const OrderId &);

		void FlushRemovedOrders();

		//! Creates order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		OnixS::FIX::Message CreateOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);
		//! Creates market order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		OnixS::FIX::Message CreateMarketOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);
		//! Creates limit order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		OnixS::FIX::Message CreateLimitOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const ScaledPrice &);
		//! Sets common fields and returns reference to preallocated order
		//! FIX-message.
		/** Uses only one object for all messages, hasn't synchronization, can
		  * called only from one thread.
		  */
		OnixS::FIX::Message & GetPreallocatedOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);
		//! Sets common fields for market orders and returns reference to
		//! preallocated market order FIX-message.
		/** Uses only one object for all messages, hasn't synchronization, can
		  * called only from one thread.
		  */
		OnixS::FIX::Message & GetPreallocatedMarketOrderMessage(
				const OrderId &,	
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);

		void Send(
				OnixS::FIX::Message &,
				Lib::TimeMeasurement::Milestones &);
		void Send(const OrderToSend &);
		void ScheduleSend(OrderToSend &);

		OrderId GetMessageOrderId(const OnixS::FIX::Message &) const;

		void NotifyOrderUpdate(
				const OnixS::FIX::Message &,
				const OrderStatus &,
				const char *operation,
				bool isOrderCompleted,
				const Lib::TimeMeasurement::Milestones::TimePoint &);

	private:

		void OnOrderNew(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderCanceled(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderRejected(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderFill(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderPartialFill(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);

	private:

		void SendThreadMain();

	private:

		CurrenexFixSession m_session;

		boost::atomic<OrderId> m_nextOrderId;
		//! @todo reimplemented with circular buffer.
		//! @todo compare insert/search speed with tree
		std::deque<Order> m_orders;
		size_t m_ordersCountReportsCounter;
		OrdersMutex m_ordersMutex;

		SendMutex m_sendMutex;
		SendCondition m_sendCondition;
		std::pair<std::vector<OrderToSend>, std::vector<OrderToSend>> m_toSend;
		std::vector<OrderToSend> *m_currentToSend;
		boost::thread m_sendThread;

		struct {
			std::unique_ptr<OnixS::FIX::Message> orderMessage;
		} m_preallocated;

	};

} } }
