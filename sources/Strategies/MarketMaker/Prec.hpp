/*******************************************************************************
 *   Created: 2018/01/30 10:51:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#pragma warning(push)
#pragma warning(disable : 4127)
#include <QtWidgets>
#pragma warning(pop)

#include "Common/Common.hpp"
#include "TradingLib/StopLoss.hpp"
#include "TradingLib/TakeProfit.hpp"
#include "Core/Balances.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Operation.hpp"
#include "Core/Position.hpp"
#include "Core/Security.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "FrontEnd/Lib/Adapters.hpp"
#include "FrontEnd/Lib/Engine.hpp"
#include "FrontEnd/Lib/ModuleApi.hpp"
#include "FrontEnd/Lib/OrderStatusNotifier.hpp"
#include "FrontEnd/Lib/Types.hpp"
#include "FrontEnd/Lib/Util.hpp"
#include "FrontEnd/Lib/Fwd.hpp"
#include "Fwd.hpp"
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "Common/Accumulators.hpp"
