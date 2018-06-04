/*******************************************************************************
 *   Created: 2017/10/15 22:29:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Common/Common.hpp"

#pragma warning(push)
#pragma warning(disable : 4127)
#include <QtWidgets>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4706)
#pragma warning(disable : 4127)
#include "GeneratedFiles/Orm/include/Trdk_FrontEnd_Lib_Orm_all_include.gen.h"
#pragma warning(pop)

#include "Common.hpp"
#include "Core/Balances.hpp"
#include "Core/Bar.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Operation.hpp"
#include "Core/Position.hpp"
#include "Core/RiskControl.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Engine/Engine.hpp"
#include "Std.hpp"
#include "Types.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#ifdef DEV_VER
#include "TradingLib/PnlContainer.hpp"
#include "Core/StrategyDummy.hpp"
#include <boost/thread.hpp>
#include <boost/uuid/random_generator.hpp>
#endif

#include "Util.hpp"
