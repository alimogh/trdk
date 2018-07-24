//
//    Created: 2018/07/24 10:44 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Interaction/Rest/Common.hpp"
#include "TradingLib/BalancesContainer.hpp"
#include "TradingLib/Util.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/Trade.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/TransactionContext.hpp"
#include "Interaction/Rest/FloodControl.hpp"
#include "Interaction/Rest/PollingTask.hpp"
#include "Interaction/Rest/Request.hpp"
#include "Interaction/Rest/Security.hpp"
#include "Interaction/Rest/Settings.hpp"
#include "Interaction/Rest/Util.hpp"
#include "Api.hpp"
#include "Fwd.hpp"
#include "Common/Crypto.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/unordered_set.hpp>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/URI.h>