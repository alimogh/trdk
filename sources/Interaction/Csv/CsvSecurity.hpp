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
					Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					bool logMarketData);

	public:

		using Base::IsTradesRequired;

		void AddTrade(
					const boost::posix_time::ptime &,
					Trader::OrderSide,
					ScaledPrice,
					Qty);

	};

} } }
