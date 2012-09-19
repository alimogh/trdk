/**************************************************************************
 *   Created: 2012/09/19 23:46:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Observer.hpp"

namespace Trader { namespace Gateway {

	class Service : public Trader::Observer {

	public:

		typedef Observer Base;

	public:

		explicit Service();
		virtual ~Service();

	};

} }
