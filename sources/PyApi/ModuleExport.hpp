/**************************************************************************
 *   Created: 2013/01/10 20:02:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "SecurityExport.hpp"
#include "Core/Module.hpp"

namespace trdk { namespace PyApi {

	class ModuleExport {

	public:

		class LogExport {
		public:
			explicit LogExport(trdk::Module::Log &);
		public:
			static void ExportClass(const char *className);
		public:
			void Debug(const char *);
			void Info(const char *);
			void Warn(const char *);
			void Error(const char *);
		private:
			trdk::Module::Log *m_log;
		};

	public:

		explicit ModuleExport(const Module &);

	public:

		static void ExportClass(const char *className);

	public:

		boost::python::str GetTag() const;
		boost::python::str GetName() const;
		LogExport GetLog() const;

	public:

		const trdk::Module & GetModule() const;

	private:

		const trdk::Module *m_module;

	};

} }
