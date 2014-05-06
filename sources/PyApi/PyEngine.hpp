/**************************************************************************
 *   Created: 2014/01/04 00:50:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "ContextExport.hpp"
#include "Engine/Context.hpp"

namespace trdk { namespace PyApi {

	class Engine
		: public trdk::Engine::Context,
		public ContextExport {

	public:

		explicit Engine(PyObject *self, const std::string &settings);
		~Engine();

	public:

		static void ExportClass(const char *className);

	public:

		void Add(const std::string &newConf);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
