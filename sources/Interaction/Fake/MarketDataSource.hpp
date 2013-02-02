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
#include "Core/Context.hpp"

namespace Trader { namespace Interaction { namespace Fake {

	class MarketDataSource : public Trader::MarketDataSource {

	public:

		class Security : public Trader::Security {

		public:

			typedef Trader::Security Base;

		public:
				
			explicit Security(
						Context &context,
						const std::string &symbol,
						const std::string &primaryExchange,
						const std::string &exchange,
						bool logMarketData)
					: Base(
						context,
						symbol,
						primaryExchange,
						exchange,
						logMarketData) {
				//...//
			}

			void AddTrade(
						const boost::posix_time::ptime &time,
						OrderSide side,
						ScaledPrice price,
						Qty qty) {
				Base::AddTrade(time, side, price, qty, true);
			}

		};

	public:

		MarketDataSource(const Lib::IniFileSectionRef &, Context::Log &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect();

	public:

		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					bool logMarketData)
				const;

	private:

		void NotificationThread();

	private:

		boost::thread_group m_threads;
		std::list<boost::shared_ptr<Security>> m_securityList;

	};

} } }
