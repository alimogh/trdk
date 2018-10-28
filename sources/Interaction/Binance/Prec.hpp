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

#define BOOST_COROUTINES_NO_DEPRECATION_WARNING

#include "Common/Common.hpp"
#include "Interaction/Rest/Common.hpp"
#include "TradingLib/BalancesContainer.hpp"
#include "Core/Context.hpp"
#include "Core/EventsLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/URI.h>

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#pragma warning(pop)