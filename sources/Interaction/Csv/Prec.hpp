/**************************************************************************
 *   Created: 2012/10/27 14:24:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/date_time.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/mem_fun.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include <fstream>

#include "Common/Assert.hpp"

#define TRADER_INTERACTION_CSV "CSV"
#define TRADER_INTERACTION_CSV_LOG_PREFFIX "[" TRADER_INTERACTION_CSV "] "
