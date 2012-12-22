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
		{
			const Log::Lock logLock(Log::GetEventsMutex());
			PyErr_Print();
		}
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

	struct Time {

		static boost::python::object Convert(
					const boost::posix_time::ptime &time) {
			static const boost::posix_time::ptime epochStart
				= boost::posix_time::from_time_t(0);
			AssertNe(epochStart, time);
			return boost::python::object((time - epochStart).total_seconds());
		}
		
		static boost::posix_time::ptime Convert(
					const boost::python::object &time) {
			Assert(time);
			try {
				return boost::posix_time::from_time_t(
					boost::python::extract<time_t>(time));
			} catch (const boost::python::error_already_set &) {
				RethrowPythonClientException(
					"Failed to convert time to time_t");
				throw;
			}
		}

	};

	struct OrderSide {

		static boost::python::object Convert(Trader::OrderSide side) {
			static_assert(
				Trader::numberOfOrderSides == 2,
				"Changed order side list.");
			switch (side) {
				case Trader::ORDER_SIDE_SELL:
					return boost::python::object('S');
				case Trader::ORDER_SIDE_BUY:
					return boost::python::object('B');
				default:
					AssertNe(int(Trader::numberOfOrderSides), side);
					return boost::python::object(' ');
			}
		}
		
		static Trader::OrderSide Convert(const boost::python::object &side) {
			Assert(side);
			try {
				switch (boost::python::extract<char>(side)) {
					case 'S':
						return Trader::ORDER_SIDE_SELL;
					case 'B':
						return Trader::ORDER_SIDE_BUY;
					default:
						throw Trader::PyApi::Error(
							"Order side can be 'B' (buy) or 'S' (sell)");
				}
			} catch (const boost::python::error_already_set &) {
				RethrowPythonClientException(
					"Failed to convert order side to Trader::OrderSide");
				throw;
			}
		}

	};

} } }
