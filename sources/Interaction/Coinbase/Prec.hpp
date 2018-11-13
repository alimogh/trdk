/*******************************************************************************
 *   Created: 2018/11/10 12:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include "Interaction/Rest/Common.hpp"
#include "TradingLib/BalancesContainer.hpp"
#include "TradingLib/Util.hpp"
#include "Core/Bar.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/Timer.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/TransactionContext.hpp"
#include "Interaction/Rest/FloodControl.hpp"
#include "Interaction/Rest/PollingTask.hpp"
#include "Interaction/Rest/Request.hpp"
#include "Interaction/Rest/Security.hpp"
#include "Interaction/Rest/Settings.hpp"
#include "Interaction/Rest/Util.hpp"
#include "Common/Crypto.hpp"
#include "Common/WebSocketConnection.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <Poco/Net/HTTPRequest.h>
