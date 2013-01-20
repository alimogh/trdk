/**************************************************************************
 *   Created: 2013/01/10 20:03:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgoExport.hpp"
#include "Core/SecurityAlgo.hpp"

using namespace Trader::PyApi;
namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

SecurityAlgoExport::LogExport::LogExport(Trader::SecurityAlgo::Log &log)
		: m_log(&log) {
	//...//
}

void SecurityAlgoExport::LogExport::Export(const char *className) {
	py::class_<LogExport>(className, py::no_init)
		.def("debug", &LogExport::Debug)
		.def("info", &LogExport::Info)
		.def("warn", &LogExport::Warn)
		.def("error", &LogExport::Error)
		.def("trading", &LogExport::Trading);
}

void SecurityAlgoExport::LogExport::Debug(const char *message) {
	m_log->Debug(message);
}

void SecurityAlgoExport::LogExport::Info(const char *message) {
	m_log->Info(message);
}

void SecurityAlgoExport::LogExport::Warn(const char *message) {
	m_log->Warn(message);
}

void SecurityAlgoExport::LogExport::Error(const char *message) {
	m_log->Error(message);
}

void SecurityAlgoExport::LogExport::Trading(const char *message) {
	m_log->Trading(message);
}

//////////////////////////////////////////////////////////////////////////

SecurityAlgoExport::SecurityAlgoExport(const Trader::SecurityAlgo &algo)
		: m_algo(&algo) {
	//...//
}

void SecurityAlgoExport::Export(const char *className) {
	
	typedef py::class_<SecurityAlgoExport, boost::noncopyable> SecurityAlgo;
	const py::scope securityAlgoClass = SecurityAlgo(className, py::no_init)
		.add_property("name", &SecurityAlgoExport::GetName)
		.add_property("tag", &SecurityAlgoExport::GetTag)
		.add_property("log", &SecurityAlgoExport::GetLog);
	
	LogExport::Export("Log");

}

py::str SecurityAlgoExport::GetTag() const {
	return m_algo->GetTag().c_str();
}

py::str SecurityAlgoExport::GetName() const {
	return m_algo->GetName().c_str();
}

SecurityAlgoExport::LogExport SecurityAlgoExport::GetLog() const {
	return LogExport(m_algo->GetLog());
}
