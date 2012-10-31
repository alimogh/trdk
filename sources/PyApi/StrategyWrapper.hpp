/**************************************************************************
 *   Created: 2012/08/07 12:53:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityWrapper.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

	//////////////////////////////////////////////////////////////////////////

	class Strategy : private boost::noncopyable {

	public:

		PyApi::Wrappers::Security security;

	public:

		Strategy() {
			//...//
		}
		virtual ~Strategy() {
			//...//
		}

	public:

		virtual boost::python::object TryToOpenPositions() = 0;
		virtual void TryToClosePositions(boost::python::object) = 0;

	};

	//////////////////////////////////////////////////////////////////////////

	struct StrategyWrap
			: public Strategy,
			public boost::python::wrapper<Strategy> {

	public:

		StrategyWrap() {
			//...//
		}

		virtual ~StrategyWrap() {
			//...///
		}

	public:

		virtual boost::python::object TryToOpenPositions() {
			return get_override("tryToOpenPositions")();
		}

		virtual void TryToClosePositions(boost::python::object position) {
			get_override("tryToClosePositions")(position);
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } }
