/**************************************************************************
 *   Created: May 26, 2012 9:44:37 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "IbSecurity.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Context.hpp"
#include "Fwd.hpp"

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {

	class Client;

} } }

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {

	class TradingSystem
		: public trdk::TradingSystem,
		public trdk::MarketDataSource {

		friend class trdk::Interaction::InteractiveBrokers::Client;

	private:
	
		struct ByAccount {
			//...//
		};
		struct BySymbol {
			//...//
		};
		struct ByOrder {
			//...//
		};
		struct BySymbolAndOrder {
			//...//
		};


		typedef Concurrency::reader_writer_lock OrdersMutex;
		typedef OrdersMutex::scoped_lock OrdersWriteLock;
		typedef OrdersMutex::scoped_lock_read OrdersReadLock;

		typedef Concurrency::reader_writer_lock PositionsMutex;
		typedef PositionsMutex::scoped_lock_read PositionsReadLock;
		typedef PositionsMutex::scoped_lock PositionsWriteLock;
		struct Position : public trdk::TradingSystem::Position {
			typedef trdk::TradingSystem::Position Base;
			Position() {
				//...//
			}
			explicit Position(
					const std::string &account,
					const Lib::Symbol &symbol,
					Qty qty)
				: Base(account, symbol, qty) {
				//...//
			}
			const std::string & GetSymbol() const {
				return symbol.GetSymbol();
			}
			const Lib::Currency & GetCurrency() const {
				return symbol.GetCurrency();
			}
		};
		typedef boost::multi_index_container<
				Position,
				boost::multi_index::indexed_by<
					boost::multi_index::ordered_non_unique<
						boost::multi_index::tag<ByAccount>,
						boost::multi_index::member<
							trdk::TradingSystem::Position,
							std::string,
							&trdk::TradingSystem::Position::account>>,
					boost::multi_index::hashed_unique<
						boost::multi_index::tag<BySymbol>,
						boost::multi_index::composite_key<
							Position,
							boost::multi_index::member<
								trdk::TradingSystem::Position,
								std::string,
								&trdk::TradingSystem::Position::account>,
							boost::multi_index::const_mem_fun<
								Position,
								const Lib::Currency &,
								&Position::GetCurrency>,
							boost::multi_index::const_mem_fun<
								Position,
								const std::string &,
								&Position::GetSymbol>>>>>
			Positions;

		struct PlacedOrder {

			OrderId id;
			trdk::Security *security;
			OrderStatusUpdateSlot callback;
			Qty filled;

			//! Updates field "filled".
			/** Should be "const" and PlacedOrder is a list item and filled
			  * is not part of list key.
			  */
			void UpdateFilled(const Qty &newValue) const {
				AssertLt(filled, newValue);
				const_cast<PlacedOrder *>(this)->filled = newValue;
			}

		};

		typedef boost::multi_index_container<
			PlacedOrder,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_non_unique<
					boost::multi_index::tag<BySymbol>,
					boost::multi_index::member<
						PlacedOrder,
						trdk::Security *,
						&PlacedOrder::security>>,
				boost::multi_index::ordered_unique<
					boost::multi_index::tag<ByOrder>,
					boost::multi_index::member<
						PlacedOrder,
						OrderId,
						&PlacedOrder::id>>>>
			PlacedOrderSet;

	public:

		explicit TradingSystem(
				const TradingMode &,
				size_t tradingSystemIndex,
				size_t marketDataSourceIndex,
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~TradingSystem();

	public:

		using trdk::TradingSystem::GetContext;

		trdk::TradingSystem::Log & GetTsLog() throw() {
			return trdk::TradingSystem::GetLog();
		}

		trdk::MarketDataSource::Log & GetMdsLog() throw() {
			return trdk::MarketDataSource::GetLog();
		}

	public:

		virtual bool IsConnected() const;
		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

	public:

		virtual const Account & GetAccount() const;

		virtual trdk::TradingSystem::Position GetBrokerPostion(
				const std::string &account,
				const trdk::Lib::Symbol &)
			const;
		virtual void TradingSystem::ForEachBrokerPostion(
				const std::string &,
				const boost::function<
					bool (const trdk::TradingSystem::Position &)> &)
			const;

	protected:

		virtual void CreateConnection(const trdk::Lib::IniSectionRef &);

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &);

		virtual OrderId SendSellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual OrderId SendBuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual void SendCancelOrder(const OrderId &);
		virtual void SendCancelAllOrders(trdk::Security &);

	private:

		void RegOrder(const PlacedOrder &);

	private:

		const bool m_isTestSource;

		OrdersMutex m_ordersMutex;
		std::unique_ptr<Client> m_client;
		PlacedOrderSet m_placedOrders;

		std::vector<boost::shared_ptr<Security>>  m_securities;
		mutable std::deque<boost::shared_ptr<Security>>
			m_unsubscribedSecurities;

		std::unique_ptr<Account> m_account;

		mutable PositionsMutex m_positionsMutex;
		std::unique_ptr<Positions> m_positions;

	};

} } }
