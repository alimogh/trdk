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
#	include <boost/thread/thread_time.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/signals2.hpp>
#	include <boost/thread/thread.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/cast.hpp>
#	include <boost/iterator/iterator_facade.hpp>
#	include <boost/tuple/tuple.hpp>
#	include <boost/format.hpp>
#	include <boost/filesystem.hpp>
#include "DisableBoostWarningsEnd.h"

#include <Windows.h>

#include "Common/Assert.hpp"

