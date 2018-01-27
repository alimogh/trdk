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

#include "Assert.hpp"
#include "Constants.h"

#include <boost/any.hpp>
#include <boost/atomic.hpp>
#include <boost/cast.hpp>
#include <boost/date_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/make_shared.hpp>
#include <boost/make_unique.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/unordered_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#if defined(BOOST_GCC)
#include <signal.h>
#endif
#include <bitset>

#include "Currency.hpp"
#include "Dll.hpp"
#include "Exception.hpp"
#include "Ini.hpp"
#include "Numeric.hpp"
#include "Spin.hpp"
#include "Symbol.hpp"
#include "SysError.hpp"
#include "TimeMeasurement.hpp"
#include "UseUnused.hpp"
#include "Util.hpp"
#include "Fwd.hpp"

#ifdef BOOST_WINDOWS
#undef SendMessage
#undef ERROR
#undef GetCurrentTime
#endif

#include "Core/Log.hpp"
#include "Core/Types.hpp"
#include "Core/Fwd.hpp"
