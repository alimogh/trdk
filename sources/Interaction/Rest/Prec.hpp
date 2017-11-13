/*******************************************************************************
 *   Created: 2017/10/09 15:09:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Fwd.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/SSLManager.h>

#ifdef DEV_VER
#include <Poco/StreamCopier.h>
#endif

#include "Common/Common.hpp"
#include "Common/Crypto.hpp"

#include "Core/Context.hpp"
#include "Core/EventsLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Timer.hpp"
#include "Core/TradingSystem.hpp"
#include "Core/TransactionContext.hpp"