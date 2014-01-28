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

Script & Script::Load(const IniSectionRef &ini) {
	return Load(ini.ReadFileSystemPath("script_file_path"));
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
			throw GetPythonClientException(
				"Internal error: Failed to initialize Python engine");
		}
		ExportApi();
		try {
			global = py::import("__main__");
			Assert(global);
		} catch (const py::error_already_set &) {
			throw GetPythonClientException(
				"Internal error: Failed to get __main__");
		}
		try {
			py::import("sys")
				.attr("path")
				.attr("append")(path.branch_path().string());
		} catch (const py::error_already_set &) {
			throw GetPythonClientException(
				"Internal error: Failed to set PYTHONPATH");
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
		throw GetPythonClientException(
			"Internal error: Failed to get __main__.__dict__");
	}

const char *s = "\n"
"import trdk\n"
"from trdkfront.wsgi import application\n"
"class Proxy(trdk.Strategy):\n"
"    def __init__(self, param, uid, symbol1, symbol2):\n"
"        trdk.Strategy.__init__(self, param)\n"
"        self.uid = uid\n"
"        self.symbol1 = symbol1\n"
"        self.symbol2 = symbol2\n"
"        application.fullTradeStrategyList.append(self)\n"
"    def getRequiredSuppliers(self):\n"
"        return 'Level 1 Updates[{0}, {1}]'.format(self.symbol1, self.symbol2)\n"
"    def onBrokerPositionUpdate(self, security, qty, isInitial):\n"
"        if isInitial is False:\n"
"            return\n"
"        if qty < 0:\n"
"            position = trdk.ShortPosition(self, security, qty, 0)\n"
"        elif qty > 0:\n"
"            position = trdk.LongPosition(self, security, qty, 0)\n"
"        else:\n"
"            return\n"
"        self.log.debug(\n"
"            'Restoring {0} {1} position: {2}...'\n"
"            .format(\n"
"                position.type,\n"
"                position.security.symbol,\n"
"                position.activeQty))\n"
"        position.restoreOpenState()\n"
"class Proxy1(Proxy):\n"
"    def __init__(self, param):\n"
"        Proxy.__init__(\n"
"            self,\n"
"            param,\n"
"            '2166230c-b4ff-4f4c-8cf1-24d7af1a3b91',\n"
"            'GBP.USD',\n"
"            'AUD.USD')\n"
"class Proxy2(Proxy):\n"
"    def __init__(self, param):\n"
"        Proxy.__init__(\n"
"            self,\n"
"            param,\n"
"            'e6bb0f76-19a4-4fec-a3be-15930d68331e',\n"
"            'EUR.USD',\n"
"            'NZD.USD')\n"
"\n";

	try {
		py::exec(s, m_global, m_global);
	} catch (const std::exception &ex) {
		throw Error(
			(boost::format("Failed to load script from %1%: \"%2%\"")
					% filePath
					% ex.what())
				.str().c_str());
	} catch (const py::error_already_set &) {
		throw GetPythonClientException(
			(boost::format("Failed to compile Python script from %1%")
					% filePath)
				.str().c_str());
	}

}

void Script::Exec(const std::string &code) {
	try {
		py::exec(code.c_str(), m_global, m_global);
	} catch (const py::error_already_set &) {
		throw GetPythonClientException("Failed to execute Python code");
	}
}

py::object Script::GetClass(
			const IniSectionRef &conf,
			Context &context,
			const char *errorWhat /*= nullptr*/) {
	try {
		return GetClass(Detail::GetModuleClassName(conf));
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
		boost::format message("Failed to load Python class %1%");
		message % name;
		throw GetPythonClientException(message.str().c_str());
	}
}

const fs::path & Script::GetFilePath() const {
	return m_filePath;
}

//////////////////////////////////////////////////////////////////////////
