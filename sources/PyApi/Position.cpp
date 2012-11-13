/**************************************************************************
 *   Created: 2012/08/07 17:31:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "SecurityWrapper.hpp"

using namespace Trader::PyApi;

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Wrappers::Security &security,
			int qty,
			double startPrice,
			const std::string &tag)
		: Trader::ShortPosition(
			security.GetSecurity().shared_from_this(),
			qty,
			security.GetSecurity().ScalePrice(startPrice),
			tag),
		Wrappers::Position(static_cast<Trader::ShortPosition &>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			Wrappers::Security &security,
			int qty,
			double startPrice,
			const std::string &tag)
		: Trader::LongPosition(
			security.GetSecurity().shared_from_this(),
			qty,
			security.GetSecurity().ScalePrice(startPrice),
			tag),
		Wrappers::Position(static_cast<Trader::LongPosition&>(*this)) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif
