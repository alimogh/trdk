/**************************************************************************
 *   Created: 2012/07/15 13:20:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	class TRADER_CORE_API MarketDataSource : private boost::noncopyable {

	public:

		class TRADER_CORE_API Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *what) throw();
		};

		class TRADER_CORE_API ConnectError : public Error {
		public:
			ConnectError() throw();
		};

	public:

		MarketDataSource();
		virtual ~MarketDataSource();

	public:

		virtual void Connect() = 0;

	public:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
					trdk::Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					bool logMarketData)
				const
				= 0;

	};

}
