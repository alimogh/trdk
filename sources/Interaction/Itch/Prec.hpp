/**************************************************************************
 *   Created: 2015/03/17 23:18:53
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
#	include <boost/asio.hpp>
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/unordered_map.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include <vector>

#include "Common/Common.hpp"

#include "Core/Fwd.hpp"
#include "Fwd.hpp"
#include "Api.h"

#include "Common/Assert.hpp"
