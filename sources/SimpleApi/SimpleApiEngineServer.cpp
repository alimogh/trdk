/**************************************************************************
 *   Created: 2014/04/29 23:27:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiEngineServer.hpp"
#include "SimpleApiEngine.hpp"
#include "Engine/Context.hpp"
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

////////////////////////////////////////////////////////////////////////////////

EngineServer::Exception::Exception(const char *what) noexcept
	: Base(what) {
	//...//
}

EngineServer::UnknownEngineError::UnknownEngineError() noexcept
	: Exception("Engine Server is not active") {
	//...//
}

EngineServer::UnknownAccountError::UnknownAccountError() noexcept
	: Exception("Account is unknown") {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class EngineServer::Implementation : private boost::noncopyable {

public:

	typedef std::vector<boost::shared_ptr<Engine>> Engines;
	Engines m_engines;

	size_t m_clientId;

	std::ofstream m_logFile;
	EventsLog m_log;
	TradingLog m_tradingLog;

	Implementation()
		: m_clientId(size_t(time(nullptr)))
		, m_log(boost::make_shared<boost::local_time::posix_time_zone>("UTC"))
		, m_tradingLog(
			boost::make_shared<boost::local_time::posix_time_zone>("UTC")) {
		//...//
	}

	boost::shared_ptr<Engine> GetEngine(const EngineId &id) {
		if (id >= m_engines.size()) {
			throw UnknownEngineError();
		}
		Assert(m_engines[id]);
		return m_engines[id];
	}

};

EngineServer::EngineServer()
	: m_pimpl(boost::make_unique<Implementation>()) {
	//...//	
}

EngineServer::~EngineServer() {
	//...//
}

void EngineServer::InitLog(const fs::path &logFilePath) {

	if (m_pimpl->m_logFile.is_open()) {
		throw Exception("Log file is already opened");
	}

	m_pimpl->m_logFile.open(
		logFilePath.c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (!m_pimpl->m_logFile) {
		throw Exception("Failed to open log file");
	}

	GetLog().EnableStream(m_pimpl->m_logFile, true);

}

EngineServer::EngineId EngineServer::CreateEngine() {
	
	std::ostringstream settingsString;
	settingsString
		<<
			"[Defaults]\n"
				"currency = USD\n"
				"security_type = FUT\n"
			"[RiskControl]\n"
				"is_enabled = no\n"
			"[TradingSystemAndMarketDataSource.x]\n"
				"module = " << GetDllFileDir().string() << "/RobotEngine\n"
				"positions = no\n"
				"client_id = " << ++m_pimpl->m_clientId << "\n"
				"account = \n";
	const IniString conf(settingsString.str());
	auto context = boost::make_shared<trdk::Engine::Context>(
		GetLog(),
		m_pimpl->m_tradingLog,
		Settings(),
		conf,
		boost::unordered_map<std::string, std::string>());
	auto engine = boost::make_shared<Engine>(context, TRADING_MODE_LIVE, 0);
	context->Start(conf);

	m_pimpl->m_engines.emplace_back(std::move(engine));

	return m_pimpl->m_engines.size() - 1;

}

void EngineServer::DestoryEngine(const EngineId &engineId) {
	m_pimpl->GetEngine(engineId); // just to check engine ID
	AssertLt(engineId, m_pimpl->m_engines.size());
	m_pimpl->m_engines.erase(m_pimpl->m_engines.begin() + engineId);
}

void EngineServer::DestoryAllEngines() {
	Implementation::Engines().swap(m_pimpl->m_engines);
}

SimpleApi::Engine & EngineServer::CheckEngine(const EngineId &id) {
	if (id < m_pimpl->m_engines.size()) {
		return *m_pimpl->m_engines[id];
	} else {
		AssertEq(m_pimpl->m_engines.size(), id);
		if (m_pimpl->m_engines.size() != id) {
			throw UnknownEngineError();
		}
		Verify(CreateEngine() == id);
		AssertEq(id + 1, m_pimpl->m_engines.size());
		return *m_pimpl->m_engines[id];
	}
}

SimpleApi::Engine & EngineServer::GetEngine(const EngineId &engineId) {
	return *m_pimpl->GetEngine(engineId);
}

const SimpleApi::Engine & EngineServer::GetEngine(
		const EngineId &engineId)
		const {
	return const_cast<EngineServer *>(this)->GetEngine(engineId);
}

EventsLog & EngineServer::GetLog() {
	return m_pimpl->m_log;
}
