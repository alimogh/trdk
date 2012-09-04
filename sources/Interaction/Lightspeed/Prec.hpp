/**************************************************************************
 *   Created: 2012/08/28 01:34:06
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
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/enable_shared_from_this.hpp>
#	include <boost/signals2.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/accumulators/accumulators.hpp>
#	include <boost/accumulators/statistics/mean.hpp>
#	include <boost/accumulators/statistics/stats.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include <stdint.h>

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
