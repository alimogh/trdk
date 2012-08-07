/**************************************************************************
 *   Created: 2012/08/07 12:53:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityWrapper.hpp"

namespace PyApi { namespace Wrappers {

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

		virtual void TryToOpenPositions() = 0;
		virtual void TryToClosePositions() = 0;

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

		virtual void TryToOpenPositions() {
			get_override("tryToOpenPositions")();
		}

		virtual void TryToClosePositions() {
			get_override("tryToClosePositions")();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }
