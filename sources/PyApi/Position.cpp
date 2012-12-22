/**************************************************************************
 *   Created: 2012/08/07 17:31:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"

using namespace Trader::PyApi;

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Export::Security &security,
			int qty,
			double startPrice,
			const std::string &tag)
		: Trader::ShortPosition(
			security.GetSecurity(),
			qty,
			security.GetSecurity()->ScalePrice(startPrice),
			tag),
		Import::ShortPosition(static_cast<Trader::ShortPosition &>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			Export::Security &security,
			int qty,
			double startPrice,
			const std::string &tag)
		: Trader::LongPosition(
			security.GetSecurity(),
			qty,
			security.GetSecurity()->ScalePrice(startPrice),
			tag),
		Import::LongPosition(static_cast<Trader::LongPosition &>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif
