/**************************************************************************
 *   Created: 2012/11/13 00:01:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "StrategyWrapper.hpp"
#include "Core/Strategy.hpp"

using namespace Trader::PyApi::Wrappers;
namespace py = boost::python;

Strategy::Strategy(
			Trader::Strategy &strategy,
			boost::shared_ptr<Trader::Security> security)
		: SecurityAlgo(strategy, security) {
	//...//
}

py::str Strategy::GetTag() const {
	return GetStrategy().GetTag().c_str();
}

py::object Strategy::PyTryToOpenPositions() {
	throw PureVirtualMethodHasNoImplementation(
		"Pure virtual method Trader.Strategy.tryToOpenPositions has no implementation");
}

void Strategy::PyTryToClosePositions(py::object) {
	throw PureVirtualMethodHasNoImplementation(
		"Pure virtual method Trader.Strategy.tryToClosePositions has no implementation");
}

Trader::Strategy & Strategy::GetStrategy() {
	return Get<Trader::Strategy>();
}

const Trader::Strategy & Strategy::GetStrategy() const {
	return Get<Trader::Strategy>();
}
