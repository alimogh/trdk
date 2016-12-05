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

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/bind.hpp>
#	include <boost/cast.hpp>
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/date_time/local_time/local_time.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/regex.hpp>
#	include <boost/variant/apply_visitor.hpp>
#	include <boost/variant/variant.hpp>
#	include <boost/tuple/tuple_comparison.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/range/adaptor/reversed.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include <bitset>

#include "Common/Assert.hpp"
