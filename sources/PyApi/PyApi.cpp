/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PyApi.hpp"
#include "ScriptEngine.hpp"
#include "Core/PositionReporterAlgo.hpp"
#include "Core/AlgoPositionState.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/PositionBundle.hpp"

namespace fs = boost::filesystem;
using namespace Trader;

PyApi::Algo::Algo(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFile &ini,
			const std::string &section)
		: Base(tag, security),
		m_scriptEngine(nullptr) {
	{
		const Settings settings = {};
		m_settings = settings;
	}
	DoSettingsUpdate(ini, section);
}

PyApi::Algo::~Algo() {
	delete m_scriptEngine;
}

const std::string & PyApi::Algo::GetName() const {
	return m_settings.algoName;
}

void PyApi::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> PyApi::Algo::TryToOpenPositions() {
	boost::shared_ptr<PositionBandle> result;
	boost::shared_ptr<Position> pos = m_scriptEngine->TryToOpenPositions();
	if (pos) {
		result.reset(new PositionBandle);
		result->Get().push_back(pos);
	}
	return result;
}

void PyApi::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		if (!p->IsOpened() || p->IsClosed() || p->IsCanceled()) {
			continue;
		}
		m_scriptEngine->TryToClosePositions(p);
	}
}

void PyApi::Algo::ReportDecision(const Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAskPrice(1),
		position.GetSecurity().GetBidPrice(1),
		position.GetSecurity().DescalePrice(position.GetOpenStartPrice()),
		position.GetPlanedQty());
}

std::auto_ptr<PositionReporter> PyApi::Algo::CreatePositionReporter() const {
	typedef PositionReporterAlgo<PyApi::Algo> Reporter;
	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return std::auto_ptr<PositionReporter>(result);
}

void PyApi::Algo::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpdate(ini, section);
}

void PyApi::Algo::DoSettingsUpdate(const IniFile &ini, const std::string &section) {

	Settings settings = {};

	const std::string algoClassName = ini.ReadKey(section, "algo", false);
	settings.algoName = ini.ReadKey(section, "name", false);

	const fs::path scriptFilePath = ini.ReadKey(section, "script_file_path", false);
	const std::string scriptFileStamp = ini.ReadKey(section, "script_file_stamp", true);

	const bool isNewScript
		= !m_scriptEngine
			|| m_scriptEngine->GetFilePath() != scriptFilePath
			|| m_scriptEngine->IsFileChanged(scriptFileStamp);

	SettingsReport report;
	AppendSettingsReport("algo", algoClassName, report);
	AppendSettingsReport("name", settings.algoName, report);
	AppendSettingsReport("tag", GetTag(), report);
	AppendSettingsReport("script_file_path", scriptFilePath, report);
	AppendSettingsReport("script_file_stamp", scriptFileStamp, report);
	AppendSettingsReport("script state", isNewScript ? "RELOADED" : "not reloaded", report);
	ReportSettings(report);

	std::unique_ptr<PyApi::ScriptEngine> scriptEngine;
	if (isNewScript) {
		scriptEngine.reset(
			new PyApi::ScriptEngine(
				scriptFilePath,
				scriptFileStamp,
				algoClassName,
				*this,
				GetSecurity()));
	}

	m_settings = settings;
	if (scriptEngine) {
		delete m_scriptEngine;
		m_scriptEngine = scriptEngine.release();
	}

}

PyApi::ScriptEngine & PyApi::Algo::GetScriptEngine() {
	Assert(m_scriptEngine);
	return *m_scriptEngine;
}

