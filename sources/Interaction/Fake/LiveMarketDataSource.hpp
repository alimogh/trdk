/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace Trader { namespace Interaction { namespace Fake {

	class LiveMarketDataSource : public Trader::LiveMarketDataSource {

	public:

		LiveMarketDataSource();
		virtual ~LiveMarketDataSource();

	public:

		virtual void Connect();

	public:

		virtual boost::shared_ptr<Security> CreateSecurity(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;
		virtual boost::shared_ptr<Security> CreateSecurity(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;

	};

} } }
