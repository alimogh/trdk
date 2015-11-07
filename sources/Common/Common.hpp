/**************************************************************************
 *   Created: 2012/07/09 14:52:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(BOOST_GCC)
#	include <signal.h>
#endif

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/noncopyable.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/signals2.hpp>
#	include <boost/thread/mutex.hpp>
#	if TRDK_CONCURRENCY_PROFILE == 0 // ::trdk::Lib::Concurrency::PROFILE_RELAX
#		include <boost/thread/condition.hpp>
#	endif
#	include <boost/thread/thread.hpp>
#	include <boost/thread/shared_mutex.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/make_shared.hpp>
#	include <boost/cast.hpp>
#	include <boost/iterator/iterator_facade.hpp>
#	include <boost/tuple/tuple.hpp>
#	include <boost/tuple/tuple_comparison.hpp>
#	include <boost/format.hpp>
#	include <boost/atomic.hpp>
#	include <boost/date_time.hpp>
#	include <boost/any.hpp>
#	include <boost/uuid/uuid.hpp>
#	include <boost/uuid/uuid_io.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Constants.h"

#include "Common/Dll.hpp"
#include "Common/Util.hpp"
#include "Common/Defaults.hpp"
#include "Common/UseUnused.hpp"
#include "Common/Foreach.hpp"
#include "Common/Ini.hpp"
#include "Common/Symbol.hpp"
#include "Common/Currency.hpp"
#include "Common/SysError.hpp"
#include "Common/Exception.hpp"
#include "Common/TimeMeasurement.hpp"
#include "Common/Spin.hpp"
#include "Common/Log.hpp"

#include "Core/Types.hpp"
