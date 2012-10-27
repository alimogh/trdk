/**************************************************************************
 *   Created: 2012/10/27 15:37:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace Trader {  namespace Interaction { namespace Csv {


	class Security : public Trader::Security {

	public:

		typedef Trader::Security Base;

	public:

		explicit Security(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings,
					bool logMarketData);
		explicit Security(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings,
					bool logMarketData);

	public:

		void SignalNewTrade(
					const boost::posix_time::ptime &,
					bool isBuy,
					ScaledPrice,
					Qty);

	};

} } }
