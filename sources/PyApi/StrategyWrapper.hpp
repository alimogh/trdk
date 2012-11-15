/**************************************************************************
 *   Created: 2012/08/07 12:53:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgoWrapper.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	class Strategy : public SecurityAlgo {

	public:

		explicit Strategy(
					Trader::Strategy &,
					boost::shared_ptr<Trader::Security>);

	public:
		
		Trader::Strategy & GetStrategy();
		const Trader::Strategy & GetStrategy() const;

	public:

		boost::python::str GetTag() const;

		boost::python::object PyTryToOpenPositions();

		void PyTryToClosePositions(boost::python::object);

	};

} } }
