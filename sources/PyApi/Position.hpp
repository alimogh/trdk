/**************************************************************************
 *   Created: 2012/11/11 03:25:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "PositionWrapper.hpp"
#include "Core/Position.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class Security;

} } }

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition
		: public Trader::ShortPosition,
		public Wrappers::Position {
	public:
		explicit ShortPosition(
					Wrappers::Security &,
					int qty,
					double startPrice,
					const std::string &);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition
		: public Trader::LongPosition,
		public Wrappers::Position {
	public:
		explicit LongPosition(
					Wrappers::Security &,
					int qty,
					double startPrice,
					const std::string &);
	};

	//////////////////////////////////////////////////////////////////////////

} }