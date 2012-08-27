/**************************************************************************
 *   Created: 2012/07/09 14:36:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Api/EWrapper.h"
#include "Api/Contract.h"
#include "Api/Order.h"
#include "Api/EPosixClientSocket.h"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/bind.hpp>
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/format.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/signals2.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/enable_shared_from_this.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include <Windows.h>
#include <WinSock2.h>

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
