/**************************************************************************
 *   Created: 2012/08/07 17:31:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"

using namespace Trader;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

//////////////////////////////////////////////////////////////////////////

PyApi::ShortPosition::ShortPosition(
			PyObject *self,
			Strategy &strategy,
			int qty,
			double startPrice)
		: Trader::ShortPosition(
			strategy.GetStrategy(),
			qty,
			strategy.GetStrategy().GetSecurity().ScalePrice(startPrice)),
		CorePositionToExport<Trader::ShortPosition>::Export(
			static_cast<Trader::ShortPosition &>(*this)),
		PythonToCoreTransit(self) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

PyApi::LongPosition::LongPosition(
			PyObject *self,
			Strategy &strategy,
			int qty,
			double startPrice)
		: Trader::LongPosition(
			strategy.GetStrategy(),
			qty,
			strategy.GetStrategy().GetSecurity().ScalePrice(startPrice)),
		CorePositionToExport<Trader::LongPosition>::Export(
			static_cast<Trader::LongPosition &>(*this)),
		PythonToCoreTransit(self) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif
