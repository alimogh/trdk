/**************************************************************************
 *   Created: 2012/10/27 15:37:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {  namespace Interaction { namespace Csv {


	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit Security(
					Context &,
					const trdk::Lib::Symbol &,
					const MarketDataSource &);

	public:

		using Base::IsTradesRequired;

		void AddTrade(
					const boost::posix_time::ptime &,
					trdk::OrderSide,
					ScaledPrice,
					Qty);

	};

} } }
