/**************************************************************************
 *   Created: 2014/05/04 13:15:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace SimpleApi { namespace Util {

	boost::posix_time::ptime ConvertEasyLanguageDateTimeToPTime(
				int date,
				int time);

} } }
