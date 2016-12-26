/**************************************************************************
 *   Created: 2016/10/15 14:10:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Test;

Test::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &instanceName,
		const IniSectionRef &)
	: Base(index, context, instanceName),
	m_stopFlag(false) {
	if (!GetContext().GetSettings().IsReplayMode()) {
		throw Error("Failed to start without Replay Mode");
	}
}

Test::MarketDataSource::~MarketDataSource() {
	//...//
}

void Test::MarketDataSource::Stop() {
	Assert(!m_stopFlag);
	m_stopFlag = true;
	m_threads.join_all();
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void Test::MarketDataSource::Connect(const IniSectionRef &) {
	if (m_threads.size()) {
		return;
	}
	m_threads.create_thread(
		[this](){
			try {
				GetLog().Debug("Started notification thread.");
				Run();
				GetLog().Debug("Notification thread is completed.");
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});
}

void Test::MarketDataSource::SubscribeToSecurities() {
	//...//
}
