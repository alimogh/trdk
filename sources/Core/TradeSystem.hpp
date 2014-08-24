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
			trdk::Context &,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &);

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
					double avgPrice)>
			OrderStatusUpdateSlot;

		struct Account {

			boost::atomic<double> cashBalance;
			boost::atomic<double> equityWithLoanValue;
			boost::atomic<double> maintenanceMargin;
			boost::atomic<double> excessLiquidity;

			Account() 
					: cashBalance(.0),
					equityWithLoanValue(.0),
					maintenanceMargin(.0),
					excessLiquidity(.0) {
				//...//
			}

		};

		struct Position {

			std::string account;
			trdk::Lib::Symbol symbol;
			trdk::Qty qty;

			Position()
					: qty(0) {
				//...//
			}
			
			explicit Position(
						const std::string &account,
						const Lib::Symbol &symbol,
						Qty qty)
					: account(account),
					symbol(symbol),
					qty(qty) {
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

		//! Broker position error.
		class TRDK_CORE_API PositionError : public Error {
		public:
			PositionError(const char *what) throw();
		};

	public:

		TradeSystem(trdk::Context &, const std::string &tag);
		virtual ~TradeSystem();

	public:

		static const char * GetStringStatus(OrderStatus);

	public:

		trdk::Context & GetContext();
		const trdk::Context & GetContext() const;

		trdk::Context::Log & GetLog() const;

		//! Identifies Trade System object by verbose name. 
		/** Trade System Tag unique, but can be empty for one of objects.
		  */
		const std::string & GetTag() const;

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &) = 0;

	public:

		//! Returns default account info.
		/** All values is unscaled. If default account hasn't been set - throws
		  * an exception.
		  * @throw trdk::TradeSystem::UnknownAccountError
		  */
		virtual const trdk::TradeSystem::Account & GetAccount() const;

		//! Returns broker position by symbol.
		virtual trdk::TradeSystem::Position GetBrokerPostion(
				const std::string &account,
				const trdk::Lib::Symbol &)
			const;
		//! Retrieves all positions for account.
		/** @param account		Account for positions search.
		  * @param predicate	Predicate, iteration will be stopped if it
		  *						returns false. Must be very fast as call can be
		  *						at lock.
		  */
		virtual void ForEachBrokerPostion(
				const std::string &account,
				const boost::function<bool (const Position &)> &predicate)
			const;

	public:

		virtual OrderId SellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Sell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual OrderId BuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId Buy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId BuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
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

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////
