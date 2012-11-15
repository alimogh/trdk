/**************************************************************************
 *   Created: 2012/11/04 21:02:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgoWrapper.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class Service : public Trader::PyApi::Wrappers::SecurityAlgo {

	public:

		explicit Service(
					Trader::Service &,
					boost::shared_ptr<Trader::Security>);

		virtual ~Service();

	public:

		Trader::Service & GetService();
		const Trader::Service & GetService() const;

	public:

		virtual boost::python::str GetTag() const;

	};

} } }
