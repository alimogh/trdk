/**************************************************************************
 *   Created: 2012/07/09 16:03:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/cast.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/variant.hpp>
#include <bitset>

#include "Version/Restrictions.h"
#if TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS || TRDK_USAGE_STOP_TIMESTAMP_MS
#include <boost/random.hpp>
#endif
