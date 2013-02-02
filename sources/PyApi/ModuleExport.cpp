/**************************************************************************
 *   Created: 2013/01/10 20:03:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ModuleExport.hpp"
#include "Core/SecurityAlgo.hpp"

using namespace Trader::PyApi;
namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

ModuleExport::LogExport::LogExport(Trader::SecurityAlgo::Log &log)
		: m_log(&log) {
	//...//
}

void ModuleExport::LogExport::ExportClass(const char *className) {
	py::class_<LogExport>(className, py::no_init)
		.def("debug", &LogExport::Debug)
		.def("info", &LogExport::Info)
		.def("warn", &LogExport::Warn)
		.def("error", &LogExport::Error);
}

void ModuleExport::LogExport::Debug(const char *message) {
	m_log->Debug(message);
}

void ModuleExport::LogExport::Info(const char *message) {
	m_log->Info(message);
}

void ModuleExport::LogExport::Warn(const char *message) {
	m_log->Warn(message);
}

void ModuleExport::LogExport::Error(const char *message) {
	m_log->Error(message);
}

//////////////////////////////////////////////////////////////////////////

ModuleExport::ModuleExport(const Trader::SecurityAlgo &algo)
		: m_algo(&algo) {
	//...//
}

void ModuleExport::ExportClass(const char *className) {
	
	typedef py::class_<ModuleExport, boost::noncopyable> SecurityAlgo;
	const py::scope securityAlgoClass = SecurityAlgo(className, py::no_init)
		.add_property("name", &ModuleExport::GetName)
		.add_property("tag", &ModuleExport::GetTag)
		.add_property("log", &ModuleExport::GetLog);
	
	LogExport::ExportClass("Log");

}

py::str ModuleExport::GetTag() const {
	return m_algo->GetTag().c_str();
}

py::str ModuleExport::GetName() const {
	return m_algo->GetName().c_str();
}

ModuleExport::LogExport ModuleExport::GetLog() const {
	return LogExport(m_algo->GetLog());
}
