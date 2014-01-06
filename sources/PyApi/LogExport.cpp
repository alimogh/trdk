/**************************************************************************
 *   Created: 2014/01/06 14:05:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "LogExport.hpp"

namespace fs = boost::filesystem;
namespace py = boost::python;
using namespace trdk;
using namespace trdk::PyApi;
using namespace trdk::PyApi::LogConfig;

namespace {

	std::ofstream eventLog;

}

void LogConfig::ExportModule(const char *modulePath, const char *moduleName) {
	
	//! @todo: export __all__
	py::object module(
		py::handle<>(py::borrowed(PyImport_AddModule(modulePath))));
	py::scope().attr(moduleName) = module;
	py::scope scope = module;
	
	py::def("enableEvents", EnableEvents);
	py::def("enableEventsToStdOut", EnableEventsToStdOut);

}

void LogConfig::EnableEvents(const std::string &logFilePathStr) {
	fs::path logFilePath(logFilePathStr);
	eventLog.close();
	fs::create_directories(logFilePath.branch_path());
	eventLog.open(
		logFilePath.c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (eventLog) {
		Log::EnableEvents(eventLog);
	}
}

void LogConfig::EnableEventsToStdOut() {
	Log::EnableEventsToStdOut();
}
