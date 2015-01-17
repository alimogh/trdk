/**************************************************************************
 *   Created: 2015/01/10 18:22:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	////////////////////////////////////////////////////////////////////////////////

	enum Y {
		Y1,
		Y2,
		numberOfYs
	};
	
	typedef boost::array<double, numberOfYs> Y;

	////////////////////////////////////////////////////////////////////////////////


} } } }
