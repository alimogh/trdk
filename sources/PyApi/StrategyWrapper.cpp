/**************************************************************************
 *   Created: 2012/11/13 00:01:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "StrategyWrapper.hpp"
#include "Service.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"

using namespace  Trader::PyApi::Wrappers;
namespace py = boost::python;

Strategy::Strategy(
			Trader::Strategy &strategy,
			boost::shared_ptr<Trader::Security> security)
		: SecurityAlgo(security),
		m_strategy(strategy) {
	//...//
}

py::str Strategy::GetTag() const {
	return m_strategy.GetTag().c_str();
}

void Strategy::PyNotifyServiceStart(py::object pyService) {
	Assert(pyService);
	try {
		const Trader::PyApi::Service &service
			= py::extract<const Trader::PyApi::Service &>(pyService);
		m_strategy.Trader::Strategy::NotifyServiceStart(service);
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert object to Trader.Service");
	}
}

py::object Strategy::PyTryToOpenPositions() {
	throw PureVirtualMethodHasNoImplementation(
		"Pure virtual method Trader.Strategy.tryToOpenPositions has no implementation");
}

void Strategy::PyTryToClosePositions(py::object) {
	throw PureVirtualMethodHasNoImplementation(
		"Pure virtual method Trader.Strategy.tryToClosePositions has no implementation");
}
