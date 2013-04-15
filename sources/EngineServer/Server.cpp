/**************************************************************************
 *   Created: 2013/02/08 12:13:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Server.hpp"
#include "Engine/Context.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::EngineServer;
using namespace trdk::Lib;

Server::Server() {
	//..//
}

void Server::Run(
			const std::string &uuid,
			const fs::path &path,
			bool isReplayMode) {
	const Lock lock(m_mutex);
	{
		const auto &index = m_engines.get<ByUuid>();
		if (index.find(uuid) != index.end()) {
			boost::format message("Engine with UUID \"%1%\" already activated");
			message % uuid;
			throw Exception(message.str().c_str());
		}
	}
	EngineInfo info = {
			uuid,
			boost::shared_ptr<Engine::Context>(
				new Engine::Context(path, isReplayMode))
		};
	info.engine->Start();
	m_engines.insert(info);
}

void Server::StopAll() {
	const Lock lock(m_mutex);
	Engines().swap(m_engines);
}
