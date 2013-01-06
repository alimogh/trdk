/**************************************************************************
 *   Created: 2012/11/25 11:20:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Detail.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace Trader::PyApi::Detail;

namespace {
	const pt::ptime epochStart = pt::from_time_t(0);
}

py::object Time::Convert(const pt::ptime &time) {
	AssertNe(epochStart, time);
	return py::object((time - epochStart).total_seconds());
}
