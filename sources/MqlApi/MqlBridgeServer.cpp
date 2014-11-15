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
#include "MqlBridge.hpp"
#include "MqlBridgeContext.hpp"


namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;


trdk::MqlApi::BridgeServer trdk::MqlApi::theBridge;


BridgeServer::Exception::Exception(const char *what) throw()
		: Base(what) {
	//...//
}
BridgeServer::UnknownEngineError::UnknownEngineError() throw()
		: Exception("MQL Bridge Server is not active") {
	//...//
}
BridgeServer::UnknownAccountError::UnknownAccountError() throw()
		: Exception("MQL Bridge Server: account is unknown") {
	//...//
}
BridgeServer::NotEnoughFreeEngineSlotsError::NotEnoughFreeEngineSlotsError()
		: Exception("Not enough free MQL Bridge Slots") {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class BridgeServer::Implementation : private boost::noncopyable {

public:

	std::ofstream m_eventLog;

	std::array<boost::shared_ptr<BridgeContext>, 10> m_engines;

	typedef boost::tuple<EngineId, boost::shared_ptr<Bridge>> Account;
	std::map<std::string, Account> m_accounts;

	Account & GetAccount(const std::string &account) {
		auto pos = m_accounts.find(account);
		if (pos == m_accounts.end()) {
			Log::Error(
				"MQL Bridge Server: account \"%1%\" is unknown.",
				account);
			throw UnknownAccountError();
		}
		return pos->second;
	}

	boost::shared_ptr<BridgeContext> GetEngine(EngineId engineId) {
		if (engineId >= m_engines.size() || !m_engines[engineId]) {
			throw UnknownEngineError();
		}
		return m_engines[engineId];
	}

	boost::shared_ptr<BridgeContext> GetEngine(const std::string &account) {
		return GetEngine(boost::get<0>(GetAccount(account)));
	}


};


BridgeServer::BridgeServer()
		: m_pimpl(new Implementation) {
	//...//	
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

BridgeServer::EngineId BridgeServer::CreateBridge(
			const std::string &twsHost,
			unsigned short twsPort,
			const std::string &account,
			const std::string &defaultExchange) {
	
	EngineId engineId = 0;
	foreach (const auto &engine, m_pimpl->m_engines) {
		if (!engine) {
			break;
		}
	}
	AssertGe(m_pimpl->m_engines.size(), engineId);
	if (engineId >= m_pimpl->m_engines.size()) {
		throw UnknownEngineError();
	}
	
	std::ostringstream settingsString;
	settingsString
		<< "[Common]\n"
				"trade_session_period_edt = 00:00:00 - 23:59:59\n"
				"wait_market_data = no\n"
			"[Defaults]\n"
				"primary_exchange = FOREX\n"
				"exchange = " << defaultExchange << "\n"
				"currency = USD\n"
			"[TradeSystem]\n"
				"module = " << GetDllFileDir().string()
					<< "/InteractiveBrokers\n"
				"positions = yes\n"
				"account = " << account << "\n"
				"client_id = " << time(nullptr) << "\n";
	if (!twsHost.empty()) {
		settingsString << "ip_address = " << twsHost << "\n";
	}
	if (twsPort) {
		settingsString << "port = " << twsPort << "\n";
	}
	boost::shared_ptr<const Ini> ini(new IniString(settingsString.str()));
	boost::shared_ptr<BridgeContext> engine(new BridgeContext(ini));
	engine->Start();

	std::swap(engine, m_pimpl->m_engines[engineId]);
	return engineId;

}

BridgeServer::EngineId BridgeServer::CreateBridge(
			const std::string &twsHost,
			unsigned short twsPort,
			const std::string &account,
			const std::string &defaultExchange,
			const std::string &expirationDate,
			double strike) {
	{
		const auto &pos = m_pimpl->m_accounts.find(account);
		if (pos != m_pimpl->m_accounts.end()) {
			Log::Warn(
				"Failed to start MQL Bridge Server:"
					" account \"%1%\" already registered.",
				account);
			return boost::get<0>(pos->second);
		}
	}
	const auto engine
		= theBridge.CreateBridge(twsHost, twsPort, account, defaultExchange);
	boost::shared_ptr<Bridge> bridge(
		new Bridge(
			*m_pimpl->GetEngine(engine),
			account,
			expirationDate,
			strike));
	m_pimpl->m_accounts[account] = boost::make_tuple(engine, bridge);
	Log::Info(
		"MQL Bridge Server registered account \"%1%\" for \"%2%\".",
		account,
		defaultExchange);
	return engine;
}

void BridgeServer::DestoryBridge(EngineId engineId) {
	m_pimpl->GetEngine(engineId).reset();
}

void BridgeServer::DestoryAllBridge() {
	foreach (auto &engine, m_pimpl->m_engines) {
		engine.reset();
	}
	m_pimpl->m_accounts.clear();
}

void BridgeServer::DestoryBridge(const std::string &account) {
	auto accounts = m_pimpl->m_accounts;
	const auto engineId = boost::get<0>(m_pimpl->GetAccount(account));
	accounts.erase(account);
	DestoryBridge(engineId);
	accounts.swap(m_pimpl->m_accounts);
	Log::Info("MQL Bridge Server unregistered account \"%1%\".", account);
}

Bridge & BridgeServer::GetBridge(const std::string &account) {
	return *boost::get<1>(m_pimpl->GetAccount(account));
}

const Bridge & BridgeServer::GetBridge(const std::string &account) const {
	return const_cast<BridgeServer *>(this)->GetBridge(account);
}
