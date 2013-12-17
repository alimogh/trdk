/**************************************************************************
 *   Created: 2012/11/05 14:20:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/SettingsReport.hpp"
#include "Errors.hpp"

namespace trdk { namespace PyApi { namespace Detail {

	ScriptError GetPythonClientException(const char *actionDescr);

	inline std::string GetModuleClassName(
				const trdk::Lib::IniSectionRef &conf) {
		try {
			return conf.ReadKey("class");
		} catch (const trdk::Lib::Ini::KeyNotExistsError &) {
			const auto dotPos = conf.GetName().find('.');
			AssertNe(std::string::npos, dotPos);
			if (std::string::npos == dotPos) {
				return conf.GetName();
			}
			return conf.GetName().substr(dotPos + 1);
		}
	}

	template<typename Module>
	void UpdateAlgoSettings(
				Module &algo,
				const trdk::Lib::IniSectionRef &conf) {
		SettingsReport::Report report;
		SettingsReport::Append("tag", algo.GetTag(), report);
		SettingsReport::Append(
			"script_file_path",
			conf.ReadKey("script_file_path"),
			report);
		SettingsReport::Append("class", GetModuleClassName(conf), report);
		algo.ReportSettings(report);
	}

	//////////////////////////////////////////////////////////////////////////

} } }
