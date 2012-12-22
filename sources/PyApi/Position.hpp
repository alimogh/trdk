/**************************************************************************
 *   Created: 2012/11/11 03:25:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Import.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class Security;

} } }

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition
		: public Trader::ShortPosition,
		public Import::ShortPosition {
	public:
		explicit ShortPosition(
					Export::Security &,
					int qty,
					double startPrice,
					const std::string &);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition
		: public Trader::LongPosition,
		public Import::LongPosition {
	public:
		explicit LongPosition(
					Export::Security &,
					int qty,
					double startPrice,
					const std::string &);
	};

	//////////////////////////////////////////////////////////////////////////

} }