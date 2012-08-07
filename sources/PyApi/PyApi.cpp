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

void PyApi::Algo::SubscribeToMarketData(
			const LiveMarketDataSource &iqFeed,
			const LiveMarketDataSource &interactiveBrokers) {

	switch (m_settings.level1DataSource) {
		case MARKET_DATA_SOURCE_IQFEED:
			iqFeed.SubscribeToMarketDataLevel1(GetSecurity());
			break;
		case MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			interactiveBrokers.SubscribeToMarketDataLevel1(GetSecurity());
			break;
		case MARKET_DATA_SOURCE_DISABLED:
			break;
		default:
			AssertFail("Unknown market data source.");
	}

	switch (m_settings.level2DataSource) {
		case MARKET_DATA_SOURCE_IQFEED:
			iqFeed.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		case MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			interactiveBrokers.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		case MARKET_DATA_SOURCE_DISABLED:
			break;
		default:
			AssertFail("Unknown market data source.");
	}

}

void PyApi::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> PyApi::Algo::TryToOpenPositions() {
	m_scriptEngine->TryToOpenPositions();
	return boost::shared_ptr<PositionBandle>();
}

void PyApi::Algo::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		m_scriptEngine->TryToClosePositions();
	}
}

void PyApi::Algo::ClosePositionsAsIs(PositionBandle &) {
	//...//
}

void PyApi::Algo::ReportDecision(const Position &) const {
	//...//
}

std::auto_ptr<PositionReporter> PyApi::Algo::CreatePositionReporter() const {
	typedef PositionReporterAlgo<PyApi::Algo> Reporter;
	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return result;
}

void PyApi::Algo::UpdateAlogImplSettings(const IniFile &ini, const std::string &section) {
	DoSettingsUpdate(ini, section);
}

void PyApi::Algo::DoSettingsUpdate(const IniFile &ini, const std::string &section) {
	
	struct Util {	
		static const char * ConvertToStr(MarketDataSource marketDataSource) {
			switch (marketDataSource) {
				case MARKET_DATA_SOURCE_IQFEED:
					return "IQFeed";
				case MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
					return "Interactive Brokers";
				default:
					AssertFail("Unknown market data source.");
				case MARKET_DATA_SOURCE_NOT_SET:
					AssertFail("Market data source not set.");
				case MARKET_DATA_SOURCE_DISABLED:
					return "disabled";
			}
		}
		static MarketDataSource ConvertStrToMarketDataSource(
					const std::string &str,
					bool isIbAvailable) {
			if (	boost::iequals(str, "IQFeed")
					|| boost::iequals(str, "IQ")
					|| boost::iequals(str, "IQFeed.net")) {
				return MARKET_DATA_SOURCE_IQFEED;
			} else if (
					boost::iequals(str, "Interactive Brokers")
					|| boost::iequals(str, "InteractiveBrokers")
					|| boost::iequals(str, "IB")) {
				if (isIbAvailable) {
					return MARKET_DATA_SOURCE_INTERACTIVE_BROKERS;
				}
			} else if (
					boost::iequals(str, "none")
					|| boost::iequals(str, "disabled")
					|| boost::iequals(str, "no")) {
				return MARKET_DATA_SOURCE_DISABLED;
			}
			throw IniFile::KeyFormatError("possible values: Interactive Brokers, IQFeed");
		}
	};

	Settings settings = {};

	const std::string algoClassName = ini.ReadKey(section, "algo", false);
	settings.algoName = ini.ReadKey(section, "name", false);

	const fs::path scriptFilePath = ini.ReadKey(section, "script_file_path", false);
	const std::string scriptFileStamp = ini.ReadKey(section, "script_file_stamp", true);
			
	settings.level1DataSource = MARKET_DATA_SOURCE_IQFEED;
	settings.level2DataSource = m_settings.level2DataSource
		?	m_settings.level2DataSource
		:	Util::ConvertStrToMarketDataSource(ini.ReadKey(section, "level2_data_source", false), true);

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
	AppendSettingsReport(
		"level1_data_source",
		Util::ConvertToStr(settings.level1DataSource),
		report);
	AppendSettingsReport(
		"level2_data_source",
		Util::ConvertToStr(settings.level2DataSource),
		report);
	ReportSettings(report);

	std::unique_ptr<PyApi::ScriptEngine> scriptEngine;
	if (isNewScript) {
		scriptEngine.reset(
			new PyApi::ScriptEngine(
				scriptFilePath,
				scriptFileStamp,
				algoClassName,
				*this,
				GetSecurity(),
				settings.level2DataSource));
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

