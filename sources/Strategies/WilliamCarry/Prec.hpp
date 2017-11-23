/*******************************************************************************
 *   Created: 2017/10/12 11:06:04
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
#include "TradingLib/StopLimit.hpp"
#include "TradingLib/StopLoss.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Position.hpp"
#include "Core/PositionOperationContext.hpp"
#include "Core/Security.hpp"
#include "Core/Strategy.hpp"
#include "FrontEnd/Lib/DropCopy.hpp"
#include "FrontEnd/Lib/Engine.hpp"
#include "FrontEnd/Lib/Util.hpp"
#include "FrontEnd/ShellLib/Module.hpp"
#include "FrontEnd/Lib/Fwd.hpp"
#include "FrontEnd/ShellLib/Fwd.hpp"
#include "Fwd.hpp"
#include <boost/unordered_map.hpp>
#include <boost/uuid/string_generator.hpp>
