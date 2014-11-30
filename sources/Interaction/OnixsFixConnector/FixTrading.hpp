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

#include "FixSession.hpp"
#include "Core/TradeSystem.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	template<Lib::Concurrency::Profile profile>
	struct FixTradingConcurrencyPolicyT {
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
	struct FixTradingConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		//! @todo TRDK-168
		typedef Lib::Concurrency::SpinMutex OrderMutex;
		typedef OrderMutex::ScopedLock OrderReadLock;
		typedef OrderMutex::ScopedLock OrderWriteLock;
		typedef Lib::Concurrency::SpinMutex SendMutex;
		typedef SendMutex::ScopedLock SendLock;
		typedef Lib::Concurrency::SpinCondition SendCondition;
	};
	typedef FixTradingConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
		FixTradingConcurrencyPolicy;

	//! FIX trade connection with OnixS C++ FIX Engine.
	class FixTrading
			: public trdk::TradeSystem,
			public OnixS::FIX::ISessionListener {

	protected:

		enum OrderType {
			ORDER_TYPE_DAY_MARKET,
			ORDER_TYPE_DAY_LIMIT,
			ORDER_TYPE_IOC_MARKET,
			ORDER_TYPE_IOC_LIMIT,
		};

		struct Order {
			//! Order must be removed from storage.
			/** Synchronization doesn't require - it provides FIX engine.
			  */
			bool isRemoved;
			OrderId id;
			std::string clOrderId;
			trdk::Security *security;
			trdk::Lib::Currency currency;
			trdk::Qty qty;
			bool isSell;
			OrderType type;
			OrderParams params;
			OrderStatusUpdateSlot callback;
			Lib::TimeMeasurement::Milestones timeMeasurement;
			Qty filledQty;
		};

	private:

		struct OrderToSend {
			OrderId id;
			std::string clOrderId;
			const trdk::Security *security;
			trdk::Lib::Currency currency;
			trdk::Qty qty;
			trdk::OrderParams params;
			bool isSell;
			Lib::TimeMeasurement::Milestones timeMeasurement;
		};

		typedef FixTradingConcurrencyPolicy::OrderMutex OrdersMutex;
		typedef FixTradingConcurrencyPolicy::OrderReadLock OrdersReadLock;
		typedef FixTradingConcurrencyPolicy::OrderWriteLock
			OrdersWriteLock;

		typedef FixTradingConcurrencyPolicy::SendMutex SendMutex;
		typedef FixTradingConcurrencyPolicy::SendLock SendLock;
		typedef FixTradingConcurrencyPolicy::SendCondition SendCondition;

	public:

		explicit FixTrading(
				size_t index,
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~FixTrading();

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

		virtual void Test();

	public:

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

		FixSession & GetSession() {
			return m_session;
		}

	protected:

		void OnOrderNew(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderCanceled(
					const OnixS::FIX::Message &,
					const OrderId &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderRejected(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &,
					const std::string &reason,
					bool isMaxOperationLimitExceeded);
		void OnOrderFill(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnOrderPartialFill(
					const OnixS::FIX::Message &,
					const Lib::TimeMeasurement::Milestones::TimePoint &);

		virtual void OnLogout() = 0;

	protected:

		//! Creates order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		OnixS::FIX::Message CreateOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const trdk::OrderParams &);
		//! Creates market order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		virtual OnixS::FIX::Message CreateMarketOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const trdk::OrderParams &)
			= 0;
		//! Creates limit order FIX-message and sets common fields.
		/** Crates new order message each time, can be called from anywhere,
		  * without synchronization.
		  */
		virtual OnixS::FIX::Message CreateLimitOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const ScaledPrice &,
				const trdk::OrderParams &)
			= 0;

		virtual void OnOrderStateChanged(
				const OnixS::FIX::Message &,
				const OrderStatus &,
				const Order &);

	protected:

		virtual Qty ParseLastShares(const OnixS::FIX::Message &) const;
		virtual Qty ParseLeavesQty(const OnixS::FIX::Message &) const;

		static OrderId GetMessageClOrderId(const OnixS::FIX::Message &);
		static OrderId GetMessageOrigClOrderId(const OnixS::FIX::Message &);

	private:

		//! Takes next free order ID.
		Order TakeOrderId(
					Security &security,
					const Lib::Currency &currency,
					const Qty &qty,
					bool isSell,
					const OrderType &,
					const OrderParams &,
					const OrderStatusUpdateSlot &,
					const Lib::TimeMeasurement::Milestones &);
		//! Deletes unsent order (at error).
		void DeleteErrorOrder(const OrderId &) throw();

		Order * FindOrder(const OrderId &);
		const Order * FindOrder(const OrderId &) const;

		void FlushRemovedOrders();

		void FillOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const Lib::Currency &,
				const Qty &,
				const std::string &account,
				const trdk::OrderParams &,
				OnixS::FIX::Message &)
			const;

		//! Sets common fields and returns reference to preallocated order
		//! FIX-message.
		/** Uses only one object for all messages, hasn't synchronization, can
		  * called only from one thread.
		  */
		OnixS::FIX::Message & GetPreallocatedOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const trdk::OrderParams &);
		//! Sets common fields for market orders and returns reference to
		//! preallocated market order FIX-message.
		/** Uses only one object for all messages, hasn't synchronization, can
		  * called only from one thread.
		  */
		OnixS::FIX::Message & GetPreallocatedMarketOrderMessage(
				const std::string &clOrderId,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const trdk::OrderParams &);

		void Send(
				OnixS::FIX::Message &,
				Lib::TimeMeasurement::Milestones &);
		void Send(const OrderToSend &);
		void ScheduleSend(OrderToSend &);

		void NotifyOrderUpdate(
				const OnixS::FIX::Message &,
				const OrderId &,
				const OrderStatus &,
				const char *operation,
				bool isOrderCompleted,
				const Lib::TimeMeasurement::Milestones::TimePoint &);

	private:

		void SendThreadMain();

	private:

		const std::string m_account;

		FixSession m_session;

		boost::atomic<OrderId> m_nextOrderId;
		//! @todo reimplemented with circular buffer.
		//! @todo compare insert/search speed with tree
		std::deque<Order> m_orders;
		size_t m_ordersCountReportsCounter;
		mutable OrdersMutex m_ordersMutex;

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
