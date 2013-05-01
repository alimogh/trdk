/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Fake {

	class MarketDataSource : public trdk::MarketDataSource {

	public:

		class Security : public trdk::Security {

		public:

			typedef trdk::Security Base;

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

		MarketDataSource(const Lib::IniFileSectionRef &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniFileSectionRef &);

	public:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
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
