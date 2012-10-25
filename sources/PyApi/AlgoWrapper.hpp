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

	class Algo : private boost::noncopyable {

	public:

		PyApi::Wrappers::Security security;

	public:

		Algo() {
			//...//
		}
		virtual ~Algo() {
			//...//
		}

	public:

		virtual boost::python::object TryToOpenPositions() = 0;
		virtual void TryToClosePositions(boost::python::object) = 0;

	};

	//////////////////////////////////////////////////////////////////////////

	struct AlgoWrap
			: public Algo,
			public boost::python::wrapper<Algo> {

	public:

		AlgoWrap() {
			//...//
		}

		virtual ~AlgoWrap() {
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
