/**************************************************************************
 *   Created: 2013/01/10 20:02:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityExport.hpp"
#include "Core/SecurityAlgo.hpp"

namespace Trader { namespace PyApi {

	class ModuleExport {

	public:

		class LogExport {
		public:
			explicit LogExport(Trader::SecurityAlgo::Log &);
		public:
			static void ExportClass(const char *className);
		public:
			void Debug(const char *);
			void Info(const char *);
			void Warn(const char *);
			void Error(const char *);
		private:
			Trader::SecurityAlgo::Log *m_log;
		};

	public:

		explicit ModuleExport(const SecurityAlgo &);

	public:

		static void ExportClass(const char *className);

	public:

		boost::python::str GetTag() const;
		boost::python::str GetName() const;
		LogExport GetLog() const;

	private:

		const Trader::SecurityAlgo *m_algo;

	};

} }
