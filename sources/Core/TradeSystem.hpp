/**************************************************************************
 *   Created: May 21, 2012 5:46:08 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Context.hpp"	//! @todo	remove context from TradeSystem as it can be
						//!			one for all contexts.
#include "Fwd.hpp"
#include "Api.h"

//////////////////////////////////////////////////////////////////////////

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	typedef boost::tuple<
			boost::shared_ptr<trdk::TradeSystem> /* can't be nullptr */,
			boost::shared_ptr<trdk::MarketDataSource> /* can be nullptr */ >
		TradeSystemFactoryResult;

	typedef trdk::TradeSystemFactoryResult (TradeSystemFactory)(
			const trdk::Lib::IniFileSectionRef &,
			trdk::Context::Log &);	//! @todo	remove context from TradeSystem
									//!			as it can be one for all
									//!			contexts.

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API TradeSystem : private boost::noncopyable {

	public:

		enum OrderStatus {
			ORDER_STATUS_PENDIGN,
			ORDER_STATUS_SUBMITTED,
			ORDER_STATUS_CANCELLED,
			ORDER_STATUS_FILLED,
			ORDER_STATUS_INACTIVE,
			ORDER_STATUS_ERROR,
			numberOfOrderStatuses
		};

		typedef boost::function<
				void(
					trdk::OrderId,
					OrderStatus,
					trdk::Qty filled,
					trdk::Qty remaining,
					double avgPrice,
					double lastPrice)>
			OrderStatusUpdateSlot;

	public:

		class TRDK_CORE_API Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class TRDK_CORE_API ConnectError : public Error {
		public:
			ConnectError() throw();
			explicit ConnectError(const char *what) throw();
		};

		class TRDK_CORE_API SendingError : public Error {
		public:
			SendingError() throw();
		};

		class TRDK_CORE_API ConnectionDoesntExistError : public Error {
		public:
			ConnectionDoesntExistError(const char *what) throw();
		};

	public:

		TradeSystem();
		virtual ~TradeSystem();

	public:

		static const char * GetStringStatus(OrderStatus);

	public:

		virtual void Connect(const trdk::Lib::IniFileSectionRef &) = 0;

	public:

		virtual OrderId SellAtMarketPrice(
				trdk::Security &,
				trdk::Qty,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Sell(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual OrderId BuyAtMarketPrice(
				trdk::Security &,
				trdk::Qty,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Buy(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice stopPrice,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual void CancelOrder(OrderId) = 0;
		virtual void CancelAllOrders(trdk::Security &) = 0;

	};

	////////////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////
