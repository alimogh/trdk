/**************************************************************************
 *   Created: 2012/07/09 11:22:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/bind.hpp>
#	include <boost/cast.hpp>
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/date_time/local_time/local_time.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/format.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string_regex.hpp>
#	include <boost/thread/shared_mutex.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
