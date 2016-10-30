/**************************************************************************
 *   Created: 2012/11/15 23:16:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/regex.hpp>
#	include <boost/accumulators/accumulators.hpp>
#	include <boost/accumulators/statistics.hpp>
#	include <boost/accumulators/statistics/rolling_mean.hpp>
#	include <boost/variant.hpp>
#	include <boost/unordered_map.hpp>
#	include <boost/uuid/string_generator.hpp>
#	include <boost/range/adaptor/reversed.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"
#include "Common/SegmentedVector.hpp"

#include <string>

#include "Common/Assert.hpp"
