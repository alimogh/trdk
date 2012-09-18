/**************************************************************************
 *   Created: May 21, 2012 5:46:08 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

//////////////////////////////////////////////////////////////////////////

namespace Trader {

	class TRADER_CORE_API TradeSystem : private boost::noncopyable {

	public:

		typedef boost::uint64_t OrderId;
		typedef boost::int32_t OrderQty;
		typedef boost::int64_t OrderScaledPrice;

		enum OrderStatus {
			ORDER_STATUS_PENDIGN,
			ORDER_STATUS_SUBMITTED,
			ORDER_STATUS_CANCELLED,
			ORDER_STATUS_FILLED,
			ORDER_STATUS_INACTIVE,
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
			ConnectError() throw();
		};

		class TRADER_CORE_API SendingError : public Error {
		public:
			SendingError() throw();
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

		virtual void Connect(const IniFile &, const std::string &section) = 0;

	public:

		virtual OrderId SellAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Sell(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderScaledPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellOrCancel(
				const Security &,
				OrderQty,
				OrderScaledPrice,
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
				OrderScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderScaledPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyOrCancel(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual void CancelOrder(OrderId) = 0;
		virtual void CancelAllOrders(const Security &) = 0;

	};

}

//////////////////////////////////////////////////////////////////////////
