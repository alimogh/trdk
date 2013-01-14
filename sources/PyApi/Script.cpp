/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Script.hpp"
#include "PositionExport.hpp"
#include "StrategyExport.hpp"
#include "SecurityExport.hpp"
#include "Import.hpp"
#include "Export.hpp"
#include "Detail.hpp"

namespace fs = boost::filesystem;
namespace py = boost::python;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::PyApi;

//////////////////////////////////////////////////////////////////////////

BOOST_PYTHON_MODULE(trader) {

	using namespace Trader::PyApi;

	//! @todo: export __all__

	py::object traderModule = py::scope();
	traderModule.attr("__path__") = "trader";

	SecurityExport::Export("Security");

	PositionExport::Export("Position");
	ShortPositionExport::Export("ShortPosition");
	LongPositionExport::Export("LongPosition");

	SecurityAlgoExport::Export("SecurityAlgo");
	StrategyExport::Export("Strategy");
	Import::Service::Export("Service");
	{
		py::object detailsModule = py::scope();
		detailsModule.attr("__path__") = "details";
		Export::Service::Export("CoreService");
	}
	{
		//! @todo: export __all__
		py::object servicesModule(
			py::handle<>(py::borrowed(PyImport_AddModule("trader.services"))));
		py::scope().attr("services") = servicesModule;
		py::scope servicesScope = servicesModule;
		Export::Services::BarService::Export("BarService");
	}

}

namespace {

	typedef boost::mutex PythonInitMutex;
	typedef PythonInitMutex::scoped_lock PythonInitLock;

	volatile long isInited = false;
	PythonInitMutex pythonInitMutex;

	void InitPython() {

		if (isInited) {
			return;
		}
		
		const PythonInitLock lock(pythonInitMutex);
		if (isInited) {
			return;
		}
		
		Py_Initialize();
		if (PyImport_AppendInittab("trader", inittrader) == -1) {
			throw Trader::PyApi::Error(
				"Failed to add trader module to the interpreter's builtin modules");
		}
		Verify(Interlocking::Exchange(isInited, true) == false);

	}

}

//////////////////////////////////////////////////////////////////////////

Script::Script(const boost::filesystem::path &filePath)
		: m_filePath(filePath) {

	InitPython();

	m_main = py::import("__main__");
	m_global = m_main.attr("__dict__");

	try {
		py::exec_file(filePath.string().c_str(), m_global, m_global);
	} catch (const std::exception &ex) {
		throw Error(
			(boost::format("Failed to load script from %1%: \"%2%\"")
					% filePath
					% ex.what())
				.str().c_str());
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			(boost::format("Failed to compile Python script from %1%")
					% filePath)
				.str().c_str());
	}

}

void Script::Exec(const std::string &code) {
	try {
		py::exec(code.c_str(), m_global, m_global);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error("Failed to execute Python code");
	}
}

//////////////////////////////////////////////////////////////////////////
