/**************************************************************************
 *   Created: 2012/11/04 21:02:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "StrategyWrapper.hpp"
#include "Core/Service.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class Service : public Trader::PyApi::Wrappers::SecurityAlgo {

	public:

		explicit Service(
					Trader::Service &service,
					boost::shared_ptr<Trader::Security> security)
				: SecurityAlgo(security),
				m_service(service) {
			//...//
		}

		virtual ~Service() {
			//...//
		}

	public:

		virtual boost::python::str GetTag() const {
			return m_service.GetTag().c_str();
		}

	private:

		Trader::Service &m_service;

	};

} } }
