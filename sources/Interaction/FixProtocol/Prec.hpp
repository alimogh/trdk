/*******************************************************************************
 *   Created: 2017/09/19 19:24:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Module.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/Trade.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/TransactionContext.hpp"
#include "Api.h"
#include "Fwd.hpp"
#include "Common/NetworkStreamClient.hpp"
#include "Common/NetworkStreamClientService.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread/recursive_mutex.hpp>
