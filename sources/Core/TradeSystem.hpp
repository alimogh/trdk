/**************************************************************************
 *   Created: May 21, 2012 5:46:08 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

class Security;
class Settings;

namespace Trader {

	class TRADER_CORE_API TradeSystem : private boost::noncopyable {

	public:

		typedef boost::int32_t OrderId;
		typedef boost::int32_t OrderQty;
		typedef boost::int64_t OrderPrice;

		enum OrderStatus {
			/** Indicates that you have transmitted the order, but have not  yet
			  * received confirmation that it has been accepted by the order
			  * destination. NOTE: This order status is not sent by TWS and should
			  * be explicitly set by the API developer when an order is submitted.
			  * ORDER_STATUS_PENDIGN_SUBMIT
			  */
			/** Indicates that you have sent a request to cancel the order but have
			  * not yet received cancel confirmation from the order destination. At
			  * this point, your order is not confirmed canceled. You may still
			  * receive an execution while your cancellation request is pending.
			  * NOTE: This order status is not sent by TWS and should be explicitly
			  * set by the API developer when an order is canceled.
			  * ORDER_STATUS_PENDIGN_CANCEL
			  */
			/** Indicates that a simulated order type has been accepted by the IB
			  * system and that this order has yet to be elected. The order is held
			  * in the IB system until the election criteria are met. At that time
			  * the order is transmitted to the order destination as specified.
			  * ORDER_STATUS_PRE_SUBMITTED
			  */
			ORDER_STATUS_PENDIGN,
			/** Indicates that your order has been accepted at the order destination
			  * and is working.
			  */
			ORDER_STATUS_SUBMITTED,
			/** Indicates that the balance of your order has been confirmed canceled
			  * by the IB system. This could occur unexpectedly when IB or the
			  * destination has rejected your order.
			  */
			ORDER_STATUS_CANCELLED,
			/** Indicates that the order has been completely filled.
			  */
			ORDER_STATUS_FILLED,
			/** Indicates that the order has been accepted by the system (simulated
			  * orders) or an exchange (native orders) but that currently the order
			  * is inactive due to system, exchange or other issues.
			  */
			ORDER_STATUS_INACTIVE,
			/** Order placing error.
			  */
			ORDER_STATUS_ERROR
		};

		typedef boost::function<
				void(
					OrderId,
					OrderStatus,
					OrderQty filled,
					OrderQty remaining,
					double avgPrice,
					double lastPrice)>
			OrderStatusUpdateSlot;

	public:

		class TRADER_CORE_API Error : public Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class TRADER_CORE_API ConnectError : public Error {
		public:
			ConnectError(const char *what) throw();
		};

		class TRADER_CORE_API ConnectionDoesntExistError : public Error {
		public:
			ConnectionDoesntExistError(const char *what) throw();
		};

	public:

		TradeSystem();
		virtual ~TradeSystem();

	public:

		static const char * GetStringStatus(OrderStatus);

	public:

		virtual void Connect(const Settings &) = 0;

		virtual bool IsCompleted(const Security &) const = 0;

	public:

		virtual OrderId SellAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Sell(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellOrCancel(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual OrderId BuyAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Buy(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyOrCancel(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual void CancelOrder(OrderId) = 0;
		virtual void CancelAllOrders(const Security &) = 0;

	public:

		virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const = 0;

	};

}
