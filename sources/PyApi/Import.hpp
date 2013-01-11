/**************************************************************************
 *   Created: 2012/11/21 09:23:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgoExport.hpp"
#include "Export.hpp"
#include "Core/Security.hpp"
#include "Core/Position.hpp"
#include "Core/Service.hpp"
#include "Core/Strategy.hpp"
#include "Errors.hpp"

namespace Trader { namespace PyApi { namespace Import {

	//////////////////////////////////////////////////////////////////////////

	class Service : public SecurityAlgoExport {

	public:

		explicit Service(Trader::Service &);
		
	public:

		static void Export(const char *className);

	public:

		bool CallOnLevel1UpdatePyMethod();
		bool CallOnNewTradePyMethod(
					const boost::python::object &time,
					const boost::python::object &price,
					const boost::python::object &qty,
					const boost::python::object &side);
		bool CallOnServiceDataUpdatePyMethod(
					const boost::python::object &service);

	protected:

		Trader::Service & GetService() {
			return Get<Trader::Service>();
		}

		const Trader::Service & GetService() const {
			return Get<Trader::Service>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } }
