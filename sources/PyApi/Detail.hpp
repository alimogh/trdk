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

	inline void LogPythonClientException() {
		{
			const Log::Lock logLock(Log::GetEventsMutex());
			PyErr_PrintEx(0);
		}
		PyErr_Clear();
	}

	template<typename Module>
	void UpdateAlgoSettings(
				Module &algo,
				const trdk::Lib::IniFileSectionRef &ini) {
		SettingsReport::Report report;
		SettingsReport::Append("tag", algo.GetTag(), report);
		SettingsReport::Append(
			"script_file_path",
			ini.ReadKey("script_file_path", false),
			report);
		SettingsReport::Append("class", ini.ReadKey("class", false), report);
		algo.ReportSettings(report);
	}

	//////////////////////////////////////////////////////////////////////////

} } }
