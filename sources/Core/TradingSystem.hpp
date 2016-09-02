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

#include "Context.hpp"	//! @todo	remove context from TradingSystem as it
						//!			can be one for all contexts.
#include "Interactor.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	typedef boost::shared_ptr<trdk::TradingSystem> (TradingSystemFactory)(
			const trdk::TradingMode &,
			size_t tradingSystemIndex,
			trdk::Context &,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &);

	struct TradingSystemAndMarketDataSourceFactoryResult {
		boost::shared_ptr<trdk::TradingSystem> tradingSystem;
		boost::shared_ptr<trdk::MarketDataSource> marketDataSource;
	};
	typedef trdk::TradingSystemAndMarketDataSourceFactoryResult (TradingSystemAndMarketDataSourceFactory)(
			const trdk::TradingMode &,
			size_t tradingSystemIndex,
			size_t marketDataSourceSystemIndex,
			trdk::Context &,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &);

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API TradingSystem : virtual public trdk::Interactor {

	public:

		typedef trdk::Interactor Base;

		typedef trdk::ModuleEventsLog Log;
		typedef trdk::ModuleTradingLog TradingLog;

		struct TradeInfo {
			std::string id;
			trdk::Qty qty;
			trdk::ScaledPrice price;
		};

		typedef boost::function<
				void(
					const trdk::OrderId &,
					const std::string &tradingSystemOrderId,
					const OrderStatus &,
					const trdk::Qty &remainingQty,
					const TradeInfo *tradeInfo)>
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
					const Qty &qty)
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
			explicit OrderParamsError(
				const char *what,
				const trdk::Qty &,
				const trdk::OrderParams &)
				throw();
		};

		class TRDK_CORE_API SendingError : public Error {
		public:
			SendingError() throw();
		};

		class TRDK_CORE_API ConnectionDoesntExistError : public Error {
		public:
			explicit ConnectionDoesntExistError(const char *what) throw();
		};

		//! Account is unknown or default account requested but hasn't been set.
		class TRDK_CORE_API UnknownAccountError : public Error {
		public:
			explicit UnknownAccountError(const char *what) throw();
		};

		//! Broker position error.
		class TRDK_CORE_API PositionError : public Error {
		public:
			explicit PositionError(const char *what) throw();
		};

	public:

		explicit TradingSystem(
				const trdk::TradingMode &,
				size_t index,
				trdk::Context &,
				const std::string &tag);
		virtual ~TradingSystem();

		TRDK_CORE_API friend std::ostream & operator <<(
				std::ostream &,
				const trdk::TradingSystem &);

	public:

		static const char * GetStringStatus(const OrderStatus &);

	public:

		const trdk::TradingMode & GetMode() const;

		size_t GetIndex() const;

		trdk::Context & GetContext();
		const trdk::Context & GetContext() const;

		trdk::TradingSystem::Log & GetLog() const throw();
		trdk::TradingSystem::TradingLog & GetTradingLog() const throw();

		//! Identifies Trading System object by verbose name. 
		/** Trading System Tag unique, but can be empty for one of objects.
		  */
		const std::string & GetTag() const;

		const std::string & GetStringId() const throw();

	public:

		virtual bool IsConnected() const = 0;
		void Connect(const trdk::Lib::IniSectionRef &);

	public:

		//! Returns default account info.
		/** All values is unscaled. If default account hasn't been set - throws
		  * an exception.
		  * @throw trdk::TradingSystem::UnknownAccountError
		  */
		virtual const trdk::TradingSystem::Account & GetAccount() const;

		//! Returns broker position by symbol.
		virtual trdk::TradingSystem::Position GetBrokerPostion(
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

		OrderId SellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId Sell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId SellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId SellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

		OrderId BuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId Buy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId BuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
		OrderId BuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &,
				trdk::RiskControlScope &,
				const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

		void CancelOrder(const OrderId &);
		void CancelAllOrders(trdk::Security &);

		virtual void Test();

	public:

		virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

	protected:

		virtual void CreateConnection(const trdk::Lib::IniSectionRef &) = 0;

	protected:

		virtual OrderId SendSellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendSell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendSellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendSellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual OrderId SendBuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendBuy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendBuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendBuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;
		virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
			= 0;

		virtual void SendCancelOrder(const OrderId &) = 0;
		virtual void SendCancelAllOrders(trdk::Security &) = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}
