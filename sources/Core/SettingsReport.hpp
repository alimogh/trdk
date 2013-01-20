/**************************************************************************
 *   Created: 2012/11/05 14:28:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Fwd.hpp"

namespace Trader { namespace SettingsReport {

	template<typename T>
	void Append(
				const std::string &name,
				const T &val,
				Report &report) {
		const Report::value_type item(
			name,
			(boost::format("%1%") % val).str());
		report.push_back(item);
	}
	
	inline void Append(
				const std::string &name,
				double val,
				Report &report) {
		const Report::value_type item(
			name,
			(boost::format("%1%") % val).str());
		report.push_back(item);
	}

	inline void Append(
				const std::string &name,
				bool val,
				Report &report) {
		Append(name, val ? "true" : "false", report);
	}

	inline void AppendPercent(
				const std::string &name,
				double val,
				Report &report) {
		const Report::value_type item(
			name,
			(boost::format("%.4f%%") % val).str());
		report.push_back(item);
	}

	inline void Append(
				const std::string &name,
				const Trader::Lib::IniFile::AbsoluteOrPercentsPrice &val,
				const Trader::Security &security,
				Report &report) {
		Append(name, val.GetStr(security.GetPriceScale()), report);
	}

} }
