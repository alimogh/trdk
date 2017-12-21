/*******************************************************************************
 *   Created: 2017/10/22 17:12:06
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
#include "TradingLib/OrderPolicy.hpp"
#include "TradingLib/PositionController.hpp"
#include "TradingLib/PositionReport.hpp"
#include "TradingLib/StopLoss.hpp"
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
#include "FrontEnd/Lib/Types.hpp"
#include "FrontEnd/Lib/Util.hpp"
#include "FrontEnd/Lib/Fwd.hpp"
#include "Fwd.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/thread/future.hpp>
#include <boost/unordered_set.hpp>
#include <boost/uuid/uuid_generators.hpp>