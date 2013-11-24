/**************************************************************************
 *   Created: 2013/01/10 20:03:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ModuleExport.hpp"
#include "ContextExport.hpp"

using namespace trdk::Lib;
using namespace trdk::PyApi;
namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

ModuleExport::LogExport::LogExport(trdk::Module::Log &log)
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

////////////////////////////////////////////////////////////////////////////////

ModuleExport::ModuleExport(const trdk::Module &module)
		: m_module(&module) {
	//...//
}

void ModuleExport::ExportClass(const char *className) {
	
	typedef py::class_<ModuleExport, boost::noncopyable> Module;
	const py::scope moduleClass = Module(className, py::no_init)
		.add_property("name", &ModuleExport::GetName)
		.add_property("tag", &ModuleExport::GetTag)
		.add_property("log", &ModuleExport::GetLog)
		.add_property("context", &ModuleExport::GetContext);
	
	LogExport::ExportClass("Log");

}

py::str ModuleExport::GetTag() const {
	return m_module->GetTag().c_str();
}

py::str ModuleExport::GetName() const {
	return m_module->GetName().c_str();
}

ModuleExport::LogExport ModuleExport::GetLog() const {
	return LogExport(m_module->GetLog());
}

ContextExport ModuleExport::GetContext() const {
	return ContextExport(m_module->GetContext());
}

const trdk::Module & ModuleExport::GetModule() const {
	return *m_module;
}

////////////////////////////////////////////////////////////////////////////////
