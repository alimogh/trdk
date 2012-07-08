/**************************************************************************
 *   Created: 2012/07/09 11:03:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/asio.hpp>
#	include <boost/circular_buffer.hpp>
#	include <boost/format.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/signals2.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
