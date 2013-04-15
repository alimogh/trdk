/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Script.hpp"
#include "BaseExport.hpp"
#include "Errors.hpp"
#include "Detail.hpp"
#include "Core/Context.hpp"

namespace fs = boost::filesystem;
namespace py = boost::python;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

//////////////////////////////////////////////////////////////////////////

namespace {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	Mutex mutex;

	std::map<fs::path, boost::shared_ptr<Script>> cache;

	py::object global;

}

//////////////////////////////////////////////////////////////////////////

Script & Script::Load(const IniFileSectionRef &ini) {
	return Load(ini.ReadFileSystemPath("script_file_path", false));
}

Script & Script::Load(const fs::path &path) {
	const Lock lock(mutex);
	const auto it = cache.find(path);
	if (it != cache.end()) {
		return *it->second;
	}
	if (!global) {
		try {
			Py_Initialize();
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Error("Internal error: Failed to initialize Python engine");
		}
		ExportApi();
		try {
			global = py::import("__main__");
			Assert(global);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Error("Internal error: Failed to get __main__");
		}
	}
	boost::shared_ptr<Script> script(new Script(global, path));
	AssertEq(path, script->GetFilePath());
	cache[script->GetFilePath()] = script;
	return *script;
	
}

Script::Script(py::object &main, const fs::path &filePath)
		: m_filePath(filePath) {

	try {
		m_global = main.attr("__dict__");
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Internal error: Failed to get __main__.__dict__");
	}

	try {
		py::exec_file(filePath.string().c_str(), m_global, m_global);
	} catch (const std::exception &ex) {
		throw Error(
			(boost::format("Failed to load script from %1%: \"%2%\"")
					% filePath
					% ex.what())
				.str().c_str());
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error(
			(boost::format("Failed to compile Python script from %1%")
					% filePath)
				.str().c_str());
	}

}

void Script::Exec(const std::string &code) {
	try {
		py::exec(code.c_str(), m_global, m_global);
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to execute Python code");
	}
}

py::object Script::GetClass(
			const IniFileSectionRef &conf,
			Context &context,
			const char *errorWhat /*= nullptr*/) {
	try {
		return GetClass(conf.ReadKey("class", false));
	} catch (const Error &ex) {
		if (!errorWhat) {
			throw;
		}
		context.GetLog().Error(ex.what());
		throw Error(errorWhat);
	}
}

py::object Script::GetClass(const std::string &name) {
	try {
		return m_global[name];
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		boost::format message("Failed to load Python class %1%");
		message % name;
		throw Error(message.str().c_str());
	}
}

const fs::path & Script::GetFilePath() const {
	return m_filePath;
}

//////////////////////////////////////////////////////////////////////////
