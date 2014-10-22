/**************************************************************************
 *   Created: 2014/10/22 16:01:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Strategies { namespace FxMb {

	boost::posix_time::time_duration ReadPositionGracePeriod(
				const Lib::IniSectionRef &conf,
				const char *key);

} } }
