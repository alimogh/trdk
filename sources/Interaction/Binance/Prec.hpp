/*******************************************************************************
 *   Created: 2018/04/07 00:2o:55
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
#include "Core/Context.hpp"
#include "Core/EventsLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/Timer.hpp"
#include "Core/Trade.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/TransactionContext.hpp"
#include "Interaction/Rest/FloodControl.hpp"
#include "Interaction/Rest/NonceStorage.hpp"
#include "Interaction/Rest/PollingTask.hpp"
#include "Interaction/Rest/Request.hpp"
#include "Interaction/Rest/Security.hpp"
#include "Interaction/Rest/Settings.hpp"
#include "Interaction/Rest/Util.hpp"
#include "Fwd.hpp"
#include "Common/Crypto.hpp"
#include "Common/WebSocketConnection.hpp"
#include <Poco/Net/HTTPRequest.h>
#include <Poco/URI.h>
