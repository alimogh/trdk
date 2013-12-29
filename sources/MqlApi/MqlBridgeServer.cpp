/**************************************************************************
 *   Created: 2013/12/16 21:47:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MqlBridgeServer.hpp"
#include "MqlBridgeContext.hpp"
#include "MqlBridgeStrategy.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;

////////////////////////////////////////////////////////////////////////////////

trdk::MqlApi::BridgeServer trdk::MqlApi::theBridge;

////////////////////////////////////////////////////////////////////////////////

class BridgeServer::Implementation : private boost::noncopyable {

public:

	std::ofstream m_eventLog;
	std::unique_ptr<BridgeContext> m_bridge;

};

////////////////////////////////////////////////////////////////////////////////

BridgeServer::BridgeServer()
		: m_pimpl(new Implementation) {
#	ifdef DEV_VER
		Log::EnableEventsToStdOut();
#	endif
}

BridgeServer::~BridgeServer() {
	delete m_pimpl;
}

void BridgeServer::InitLog(const fs::path &logFilePath) {
	m_pimpl->m_eventLog.close();
	fs::create_directories(logFilePath.branch_path());
	m_pimpl->m_eventLog.open(
		logFilePath.c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (m_pimpl->m_eventLog) {
		Log::EnableEvents(m_pimpl->m_eventLog);
	}
}

bool BridgeServer::IsActive() const {
	return m_pimpl->m_bridge;
}

void BridgeServer::Run() {
	Assert(!m_pimpl->m_bridge);
	const std::string settingsString(
		"[Common]\n"
			"trade_session_period_edt = 00:00:00 - 23:59:59\n"
			"wait_market_data = no\n"
		"[Defaults]\n"
			"primary_exchange = FOREX\n"
			"exchange = IDEALPRO\n"
			"currency = USD\n"
		"[TradeSystem]\n"
			"module = InteractiveBrokers\n"
			"positions = yes\n"
		"[Strategy.MqlBridge]\n"
			"module = a:/Projects/TRDK/output/x86/bin/MqlApi\n"
			"standalone = true\n");
	boost::shared_ptr<const Ini> ini(new IniString(settingsString));
	std::unique_ptr<BridgeContext> engine(new BridgeContext(ini));
	engine->Start();
	std::swap(engine, m_pimpl->m_bridge);
}

void BridgeServer::Stop() {
	Assert(m_pimpl->m_bridge);
	m_pimpl->m_bridge.reset();
}

BridgeStrategy & BridgeServer::GetStrategy() {
	if (!m_pimpl->m_bridge) {
		throw Exception("MQL Bridge Server is not active");
	}
	return *m_pimpl->m_bridge->GetStrategy();
}


const BridgeStrategy & BridgeServer::GetStrategy() const {
	if (!m_pimpl->m_bridge) {
		throw Exception("MQL Bridge Server is not active");
	}
	return *m_pimpl->m_bridge->GetStrategy();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateMqlBridgeStrategy(
			trdk::Context &context,
			const std::string &tag,
			const IniSectionRef &) {
	if (!dynamic_cast<BridgeContext *>(&context)) {
		throw ModuleError("Wrong context type for MQL Bridge Strategy");
	}
	auto &bridge = *boost::polymorphic_downcast<BridgeContext *>(&context);
	return bridge.InitStrategy(tag);
}

////////////////////////////////////////////////////////////////////////////////
