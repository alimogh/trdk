/**************************************************************************
 *   Created: 2012/11/05 14:20:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/SettingsReport.hpp"
#include "Script.hpp"
#include "Errors.hpp"

namespace Trader { namespace PyApi { namespace Detail {

	inline void RethrowPythonClientException(const char *what) {
		PyErr_Print();
		throw Trader::PyApi::Error(what);
	}

#	pragma warning(push)
#	pragma warning(disable: 4702)
	inline Script * LoadScript(const Trader::Lib::IniFileSectionRef &ini) {
		try {
			return new Script(ini.ReadKey("script_file_path", false));
		} catch (const boost::python::error_already_set &) {
			RethrowPythonClientException("Failed to load script");
			throw;
		}
	}
#	pragma warning(pop)

	inline boost::python::object GetPyClass(
				Script &script,
				const Trader::Lib::IniFileSectionRef &ini,
				const char *errorWhat) {
		try {
			return script.GetGlobal()[ini.ReadKey("class", false)];
		} catch (const boost::python::error_already_set &) {
			RethrowPythonClientException(errorWhat);
			throw;
		}
	}

	template<typename Module>
	void UpdateAlgoSettings(
				Module &algo,
				const Trader::Lib::IniFileSectionRef &ini) {
		AssertEq(
			ini.ReadKey("script_file_path", false),
			algo.m_script->GetFilePath());
		SettingsReport::Report report;
		SettingsReport::Append("tag", algo.GetTag(), report);
		SettingsReport::Append(
			"script_file_path",
			ini.ReadKey("script_file_path", false),
			report);
		SettingsReport::Append("class", ini.ReadKey("class", false), report);
		algo.ReportSettings(report);
	}

} } }
