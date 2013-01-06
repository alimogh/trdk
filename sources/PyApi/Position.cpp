/**************************************************************************
 *   Created: 2012/08/07 17:31:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "Import.hpp"

using namespace Trader::PyApi;

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Strategy &strategy,
			int qty,
			double startPrice)
		: Trader::ShortPosition(
			strategy.GetStrategy(),
			qty,
			strategy.GetStrategy().GetSecurity().ScalePrice(startPrice)),
		Import::ShortPosition(static_cast<Trader::ShortPosition &>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			Strategy &strategy,
			int qty,
			double startPrice)
		: Trader::LongPosition(
			strategy.GetStrategy(),
			qty,
			strategy.GetStrategy().GetSecurity().ScalePrice(startPrice)),
		Import::LongPosition(static_cast<Trader::LongPosition &>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif
