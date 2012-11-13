/**************************************************************************
 *   Created: 2012/11/04 21:03:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityWrapper.hpp"
#include "Detail.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class SecurityAlgo : boost::noncopyable {

	public:

		PyApi::Wrappers::Security security;

	public:

		explicit SecurityAlgo(boost::shared_ptr<Trader::Security> security)
				: security(security) {
			//...//
		}

		virtual ~SecurityAlgo() {
			//...//
		}

	public:

		virtual boost::python::str GetTag() const = 0;

		boost::python::str PyGetName() const {
			throw PureVirtualMethodHasNoImplementation(
				"Pure virtual method Trader.SecurityAlgo.getName has no implementation");
		}

	};

} } }
