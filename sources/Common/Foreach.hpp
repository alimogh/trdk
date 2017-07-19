/**************************************************************************
 *   Created: 2012/07/08 12:16:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/foreach.hpp>

namespace boost {
namespace BOOST_FOREACH = foreach;
}

#define foreach BOOST_FOREACH
#define foreach_reversed BOOST_REVERSE_FOREACH
