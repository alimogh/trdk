/**************************************************************************
 *   Created: 2016/12/14 02:50:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>

#include "Common/Common.hpp"
#include "Core/Balances.hpp"
#include "Core/DropCopy.hpp"
#include "Core/EventsLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Operation.hpp"
#include "Core/Position.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Core/Timer.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "Common/Accumulators.hpp"
