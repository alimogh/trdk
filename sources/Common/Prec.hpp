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
#	include <boost/format.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/regex.hpp>
#	include <boost/atomic.hpp>
#	include <boost/date_time/posix_time/ptime.hpp>
#	include <boost/date_time/posix_time/posix_time_io.hpp>
#	include <boost/uuid/uuid.hpp>
#include "DisableBoostWarningsEnd.h"

#include <iomanip>
#include <iostream>

#ifdef BOOST_WINDOWS
#	include <Windows.h>
#endif

#include "Common/Constants.h"

#include "Common/Assert.hpp"

