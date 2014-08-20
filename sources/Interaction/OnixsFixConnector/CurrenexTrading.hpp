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
		};

		
#	ifdef BOOST_WINDOWS
		typedef Concurrency::reader_writer_lock OrdersMutex;
		typedef OrdersMutex::scoped_lock_read OrdersReadLock;
		typedef OrdersMutex::scoped_lock OrdersWriteLock;
#	else
		typedef boost::shared_mutex OrdersMutex;
		typedef boost::shared_lock<OrdersMutex> OrdersReadLock;
		typedef boost::unique_lock<OrdersMutex> OrdersWriteLock;
#	endif

	public:

		explicit CurrenexTrading(
					const std::string &tag,
					const Lib::IniSectionRef &,
					Context::Log &);
		virtual ~CurrenexTrading();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

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

	private:

		//! Takes next free order ID.
		OrderId TakeOrderId(const OrderStatusUpdateSlot &);
		//! Deletes unsent order (at error).
		void DeleteErrorOrder(const OrderId &) throw();

		Order * FindOrder(const OrderId &);
		OrderStatusUpdateSlot FindOrderCallback(
					const OrderId &,
					const char *operation,
					bool isOrderCompleted);

		void FlushRemovedOrders();

		//! Creates order FIX-message and sets common fields.
		OnixS::FIX::Message CreateOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);
		//! Creates market order FIX-message and sets common fields.
		OnixS::FIX::Message CreateMarketOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);
		//! Creates limit order FIX-message and sets common fields.
		OnixS::FIX::Message CreateLimitOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &,
				const ScaledPrice &);

		OrderId GetMessageOrderId(const OnixS::FIX::Message &) const;

		void NotifyOrderUpdate(
				const OnixS::FIX::Message &,
				const OrderStatus &,
				const char *operation,
				bool isOrderCompleted);

	private:

		void OnOrderNew(const OnixS::FIX::Message &);
		void OnOrderCanceled(const OnixS::FIX::Message &);
		void OnOrderRejected(const OnixS::FIX::Message &);
		void OnOrderFill(const OnixS::FIX::Message &);
		void OnOrderPartialFill(const OnixS::FIX::Message &);

	private:

		Context::Log &m_log;
		CurrenexFixSession m_session;

		OrderId m_nextOrderId;
		//! @todo reimplemented with circular buffer.
		//! @todo compare insert/search speed with tree
		std::deque<Order> m_orders;
		size_t m_ordersCountReportsCounter;
		OrdersMutex m_ordersMutex;

	};

} } }
