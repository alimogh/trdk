/*******************************************************************************
 *   Created: 2016/02/07 03:30:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MockContext.hpp"
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

namespace {
	Context::Log contextLog;
	Context::TradingLog tradingLog;
}

MockContext::MockContext()
	: Context(
		contextLog,
		tradingLog,
		Settings(false, boost::filesystem::path()),
		boost::posix_time::microsec_clock::local_time()) {
	//...//
}

MockContext::~MockContext() {
	//...//
}
