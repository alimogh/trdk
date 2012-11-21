/**************************************************************************
 *   Created: 2012/11/21 20:15:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Import.hpp"
#include "Service.hpp"

using namespace Trader::PyApi::Import;
namespace py = boost::python;

void SecurityAlgo::CallNotifyServiceStartPyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			m_algo.Trader::SecurityAlgo::NotifyServiceStart(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			m_algo.Trader::SecurityAlgo::NotifyServiceStart(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert object to Trader.Service");
	}
}
