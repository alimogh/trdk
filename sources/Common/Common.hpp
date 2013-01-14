/**************************************************************************
 *   Created: 2012/07/09 14:52:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/thread/thread_time.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/signals2.hpp>
#	include <boost/thread/mutex.hpp>
#	include <boost/thread/shared_mutex.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/cast.hpp>
#	include <boost/iterator/iterator_facade.hpp>
#	include <boost/tuple/tuple_io.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Dll.hpp"
#include "Common/Interlocking.hpp"
#include "Common/Util.hpp"
#include "Common/Interlocking.hpp"
#include "Common/Defaults.hpp"
#include "Common/UseUnused.hpp"
#include "Common/Foreach.hpp"
#include "Common/IniFile.hpp"
#include "Common/FileSystemChangeNotificator.hpp"
#include "Common/SysError.hpp"
#include "Common/Exception.hpp"

#include "Core/Types.hpp"
#include "Core/Log.hpp"
#include "Core/Fwd.hpp"
