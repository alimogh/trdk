/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"

namespace Trader { namespace Interaction { namespace Fake {

	class LiveMarketDataSource : public Trader::LiveMarketDataSource {

	public:

		class Security : public Trader::Security {

		public:

			typedef Trader::Security Base;

		public:
				
			explicit Security(
						boost::shared_ptr<Trader::TradeSystem> ts,
						const std::string &symbol,
						const std::string &primaryExchange,
						const std::string &exchange,
						boost::shared_ptr<const Trader::Settings> settings,
						bool logMarketData)
					: Base(ts, symbol, primaryExchange, exchange, settings, logMarketData) {
				//...//
			}
			
			explicit Security(
						const std::string &symbol,
						const std::string &primaryExchange,
						const std::string &exchange,
						boost::shared_ptr<const Trader::Settings> settings,
						bool logMarketData)
					: Base(symbol, primaryExchange, exchange, settings, logMarketData) {
				//...//
			}


			~Security() {
				//...//
			}

			void SetFirstUpdate(bool isBuy, ScaledPrice price, Qty qty) {
				Base::SetFirstUpdate(isBuy, price, qty);
			}

		};

	public:

		LiveMarketDataSource();
		virtual ~LiveMarketDataSource();

	public:

		virtual void Connect();

	public:

		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;
		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;

	private:

		void NotificationThread();

	private:

		boost::thread_group m_threads;
		mutable std::list<boost::shared_ptr<Security>> m_securityList;

	};

} } }
