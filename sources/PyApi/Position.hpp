/**************************************************************************
 *   Created: 2012/11/11 03:25:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Import.hpp"
#include "Strategy.hpp"

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class ShortPosition
		: public Trader::ShortPosition,
		public Import::ShortPosition {
	public:
		explicit ShortPosition(
					PyApi::Strategy &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

	class LongPosition
		: public Trader::LongPosition,
		public Import::LongPosition {
	public:
		explicit LongPosition(
					PyApi::Strategy &,
					int qty,
					double startPrice);
	};

	//////////////////////////////////////////////////////////////////////////

} }