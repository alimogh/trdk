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
#include "Interactor.hpp"
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
			const trdk::Lib::IniSectionRef &,
			trdk::Context::Log &);	//! @todo	remove context from TradeSystem
									//!			as it can be one for all
									//!			contexts.

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API TradeSystem : virtual public trdk::Interactor {

	public:

		typedef trdk::Interactor Base;

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

		struct Account {

			volatile double cashBalance;
			volatile double equityWithLoanValue;
			volatile double maintenanceMargin;
			volatile double excessLiquidity;

			Account() 
					: cashBalance(.0),
					equityWithLoanValue(.0),
					maintenanceMargin(.0),
					excessLiquidity(.0) {
				//...//
			}

		};

	public:

		class TRDK_CORE_API Error : public Base::Error {
		public:
			explicit Error(const char *what) throw();
		};

		class TRDK_CORE_API OrderParamsError : public Error {
		public:
			OrderParamsError(
						const char *what,
						trdk::Qty,
						const trdk::OrderParams &)
					throw();
		};

		class TRDK_CORE_API SendingError : public Error {
		public:
			SendingError() throw();
		};

		class TRDK_CORE_API ConnectionDoesntExistError : public Error {
		public:
			ConnectionDoesntExistError(const char *what) throw();
		};

		//! Account is unknown or default account requested but hasn't been set.
		class TRDK_CORE_API UnknownAccountError : public Error {
		public:
			UnknownAccountError(const char *what) throw();
		};


	public:

		TradeSystem();
		virtual ~TradeSystem();

	public:

		static const char * GetStringStatus(OrderStatus);

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &) = 0;

	public:

		//! Returns default account info.
		/** All values is unscaled. If default account hasn't been set - throws
		  * an exception.
		  * @throw trdk::TradeSystem::UnknownAccountError
		  */
		virtual const trdk::TradeSystem::Account & GetAccount() const = 0;

	public:

		virtual OrderId SellAtMarketPrice(
				trdk::Security &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Sell(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual OrderId BuyAtMarketPrice(
				trdk::Security &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Buy(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual void CancelOrder(OrderId) = 0;
		virtual void CancelAllOrders(trdk::Security &) = 0;

	protected:

		//! Validates order parameters and throws an exception if it has errors.
		/** @throw trdk::OrderPatams
		  */
		virtual void Validate(Qty, const trdk::OrderParams &, bool isIoc) const;

	};

	////////////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////
