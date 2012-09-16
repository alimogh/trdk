/**************************************************************************
 *   Created: 2012/09/16 17:07:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace Trader {  namespace Interaction { namespace Enyx {


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

		void OnOrderAdd();
		void OnOrderDel();

	};


} } }
