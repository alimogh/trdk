/**************************************************************************
 *   Created: 2013/01/10 20:03:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgoExport.hpp"
#include "Import.hpp"
#include "Core/SecurityAlgo.hpp"
#include "Service.hpp"
#include "Errors.hpp"
#include "Detail.hpp"

using namespace Trader::PyApi;
namespace py = boost::python;

SecurityAlgoExport::SecurityAlgoExport(Trader::SecurityAlgo &algo)
		: m_security(algo.GetSecurity().shared_from_this()),
		m_algo(algo) {
	//...//
}

void SecurityAlgoExport::Export(const char *className) {
	py::class_<SecurityAlgoExport, boost::noncopyable>(className, py::no_init)
		.add_property("tag", &SecurityAlgoExport::GetTag)
 		.def_readonly("security", &SecurityAlgoExport::m_security)
 		.def("getName", pure_virtual(&SecurityAlgoExport::CallGetNamePyMethod));
}

py::str SecurityAlgoExport::GetTag() const {
	return m_algo.GetTag().c_str();
}

py::str SecurityAlgoExport::CallGetNamePyMethod() const {
	throw PureVirtualMethodHasNoImplementation(
		"Pure virtual method trader.SecurityAlgo.getName"
			" has no implementation");
}

void SecurityAlgoExport::CallOnServiceStartPyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			m_algo.Trader::SecurityAlgo::OnServiceStart(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			m_algo.Trader::SecurityAlgo::OnServiceStart(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw PyApi::Error("Failed to convert object to Trader::Service");
	}
}
