/**************************************************************************
 *   Created: 2014/08/10 21:09:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include <boost/uuid/uuid_generators.hpp>

#ifdef BOOST_WINDOWS
#undef SendMessage
#undef ERROR
#undef GetCurrentTime
#endif
