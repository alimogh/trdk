/**************************************************************************
 *   Created: 2013/05/01 02:29:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "DisableBoostWarningsBegin.h"
#	include <boost/algorithm/string.hpp>
#	include <boost/thread/thread.hpp>
#	include <boost/thread/recursive_mutex.hpp>
#	include <boost/format.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/regex.hpp>
#	include <boost/atomic.hpp>
#	include <boost/date_time/posix_time/ptime.hpp>
#	include <boost/date_time/posix_time/posix_time_io.hpp>
#	include <boost/date_time/gregorian/gregorian_io.hpp>
#	include <boost/uuid/uuid.hpp>
#	include <boost/unordered_map.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/mem_fun.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/make_shared.hpp>
#	include <boost/make_unique.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/asio.hpp>
#	include <boost/signals2.hpp>
#include "DisableBoostWarningsEnd.h"

#include <iomanip>
#include <iostream>

#ifdef BOOST_WINDOWS
#	include <Windows.h>
#	undef SendMessage
#	undef ERROR
#	undef GetCurrentTime
#endif

#include "Common/Constants.h"

#include "Core/Fwd.hpp"

#include "Common/Assert.hpp"

