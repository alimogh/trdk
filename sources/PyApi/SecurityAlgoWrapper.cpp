/**************************************************************************
 *   Created: 2012/11/15 14:31:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgoWrapper.hpp"
#include "Service.hpp"

using namespace Trader::PyApi::Wrappers;
namespace py = boost::python;

void SecurityAlgo::PyNotifyServiceStart(py::object pyService) {
	Assert(pyService);
	try {
		const Trader::PyApi::Service &service
			= py::extract<const Trader::PyApi::Service &>(pyService);
		m_algo.Trader::SecurityAlgo::NotifyServiceStart(service);
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert object to Trader.Service");
	}
}
