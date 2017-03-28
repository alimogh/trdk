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
#include "SimpleApiBridgeServer.hpp"
#include "SimpleApiBridge.hpp"
#include "SimpleApiBridgeContext.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

////////////////////////////////////////////////////////////////////////////////

BridgeServer::Exception::Exception(const char *what) throw()
		: Base(what) {
	//...//
}

BridgeServer::UnknownEngineError::UnknownEngineError() throw()
		: Exception("Bridge Server is not active") {
	//...//
}

BridgeServer::UnknownAccountError::UnknownAccountError() throw()
		: Exception("Bridge Server: account is unknown") {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class BridgeServer::Implementation : private boost::noncopyable {

public:

	std::ofstream m_eventLog;

	typedef std::vector<boost::shared_ptr<Bridge>> Bridges;
	Bridges m_bridges;

	size_t m_clientId;

	bool m_isLogInited;

	Implementation()
			: m_clientId(size_t(time(nullptr))),
			m_isLogInited(false) {
		//...//
	}

	boost::shared_ptr<Bridge> GetBridge(const BridgeId &id) {
		if (id >= m_bridges.size()) {
			throw UnknownEngineError();
		}
		Assert(m_bridges[id]);
		return m_bridges[id];
	}

};

BridgeServer::BridgeServer()
		: m_pimpl(new Implementation) {
	//...//	
}

BridgeServer::~BridgeServer() {
	delete m_pimpl;
}

bool BridgeServer::IsLogInited() const {
	return m_pimpl->m_isLogInited;
}

void BridgeServer::InitLog(const fs::path &logFilePath) {
	m_pimpl->m_isLogInited = true;
	m_pimpl->m_eventLog.close();
	fs::create_directories(logFilePath.branch_path());
	m_pimpl->m_eventLog.open(
		logFilePath.c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (m_pimpl->m_eventLog) {
		Log::EnableEvents(m_pimpl->m_eventLog);
		Log::Info(
			"Started:"
				" Build: " TRDK_BUILD_IDENTITY "."
				" Build time: " __TIME__ " " __DATE__ ".");
		Log::Info(
			"Local time: %1%.",
			boost::posix_time::microsec_clock::local_time());
	}
}

BridgeServer::BridgeId BridgeServer::CreateBridge(const BridgeId *id) {
	
	std::ostringstream settingsString;
	settingsString
		<< "[Common]\n"
				"trade_session_period_edt = 00:00:00 - 23:59:59\n"
				"wait_market_data = no\n"
			"[Defaults]\n"
				"primary_exchange = FOREX\n"
				"exchange = GLOBEX\n"
				"currency = USD\n"
			"[TradeSystem]\n"
				"module = " << GetDllWorkingDir().string() << "/TrdkTwsBridge\n"
				"positions = yes\n"
				"client_id = " << ++m_pimpl->m_clientId << "\n"
				"account = \n";
	boost::shared_ptr<const Ini> ini(new IniString(settingsString.str()));
	boost::shared_ptr<BridgeContext> context(new BridgeContext(ini));
	boost::shared_ptr<Bridge> bridge(new Bridge(context));
	context->Start();

	if (!id) {
		const BridgeId newBridgeId = m_pimpl->m_bridges.size();
		m_pimpl->m_bridges.push_back(bridge);
		return newBridgeId;
	} else {
		m_pimpl->m_bridges[*id] = bridge;
		return *id;
	}

}

void BridgeServer::DestoryBridge(const BridgeId &bridgeId) {
	m_pimpl->GetBridge(bridgeId); // just to check bridge ID
	AssertLt(bridgeId, m_pimpl->m_bridges.size());
	m_pimpl->m_bridges.erase(m_pimpl->m_bridges.begin() + bridgeId);
}

void BridgeServer::DestoryAllBridge() {
	Implementation::Bridges().swap(m_pimpl->m_bridges);
}

Bridge & BridgeServer::CheckBridge(const BridgeId &id) {
	if (id < m_pimpl->m_bridges.size()) {
		if (m_pimpl->m_bridges[id]->CheckActive()) {
			return *m_pimpl->m_bridges[id];
		}
		Verify(CreateBridge(&id) == id);
		return *m_pimpl->m_bridges[id];
	} else {
		AssertEq(m_pimpl->m_bridges.size(), id);
		if (m_pimpl->m_bridges.size() != id) {
			throw UnknownEngineError();
		}
		Verify(CreateBridge() == id);
		AssertEq(id + 1, m_pimpl->m_bridges.size());
		return *m_pimpl->m_bridges[id];
	}
}

Bridge & BridgeServer::GetBridge(const BridgeId &bridgeId) {
	return *m_pimpl->GetBridge(bridgeId);
}

const Bridge & BridgeServer::GetBridge(const BridgeId &bridgeId) const {
	return const_cast<BridgeServer *>(this)->GetBridge(bridgeId);
}
