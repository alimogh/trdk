/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PyApi.hpp"
#include "Core/PositionReporterAlgo.hpp"
#include "Core/AlgoPositionState.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/PositionBundle.hpp"

PyApi::Algo::Algo(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFile &ini,
			const std::string &section)
		: Base(tag, security) {
	DoSettingsUpdate(ini, section);
}

PyApi::Algo::~Algo() {
	//...//
}

const std::string & PyApi::Algo::GetName() const {
	return m_settings.algoName;
}

void PyApi::Algo::SubscribeToMarketData(
			const LiveMarketDataSource &iqFeed,
			const LiveMarketDataSource &interactiveBrokers) {

	switch (m_settings.level1DataSource) {
		case Settings::MARKET_DATA_SOURCE_IQFEED:
			iqFeed.SubscribeToMarketDataLevel1(GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			interactiveBrokers.SubscribeToMarketDataLevel1(GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_DISABLED:
			break;
		default:
			AssertFail("Unknown market data source.");
	}

	switch (m_settings.level2DataSource) {
		case Settings::MARKET_DATA_SOURCE_IQFEED:
			iqFeed.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
			interactiveBrokers.SubscribeToMarketDataLevel2(GetSecurity());
			break;
		case Settings::MARKET_DATA_SOURCE_DISABLED:
			break;
		default:
			AssertFail("Unknown market data source.");
	}

}

void PyApi::Algo::Update() {
	AssertFail("Strategy logic error - method \"Update\" has been called");
}

boost::shared_ptr<PositionBandle> PyApi::Algo::TryToOpenPositions() {
	return boost::shared_ptr<PositionBandle>();
}

void PyApi::Algo::TryToClosePositions(PositionBandle &) {

}

void PyApi::Algo::ClosePositionsAsIs(PositionBandle &) {

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
		static const char * ConvertToStr(Settings::MarketDataSource marketDataSource) {
			switch (marketDataSource) {
				case Settings::MARKET_DATA_SOURCE_IQFEED:
					return "IQFeed";
				case Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
					return "Interactive Brokers";
				default:
					AssertFail("Unknown market data source.");
				case Settings::MARKET_DATA_SOURCE_NOT_SET:
					AssertFail("Market data source not set.");
				case Settings::MARKET_DATA_SOURCE_DISABLED:
					return "disabled";
			}
		}
		static Settings::MarketDataSource ConvertStrToMarketDataSource(
					const std::string &str,
					bool isIbAvailable) {
			if (	boost::iequals(str, "IQFeed")
					|| boost::iequals(str, "IQ")
					|| boost::iequals(str, "IQFeed.net")) {
				return Settings::MARKET_DATA_SOURCE_IQFEED;
			} else if (
					boost::iequals(str, "Interactive Brokers")
					|| boost::iequals(str, "InteractiveBrokers")
					|| boost::iequals(str, "IB")) {
				if (isIbAvailable) {
					return Settings::MARKET_DATA_SOURCE_INTERACTIVE_BROKERS;
				}
			} else if (
					boost::iequals(str, "none")
					|| boost::iequals(str, "disabled")
					|| boost::iequals(str, "no")) {
				return Settings::MARKET_DATA_SOURCE_DISABLED;
			}
			throw IniFile::KeyFormatError("possible values: Interactive Brokers, IQFeed");
		}
	};

	Settings settings = {};

	settings.algoName = ini.ReadKey(section, "name", false);

	settings.scriptFile = ini.ReadKey(section, "script_file", false);
			
	settings.positionOpenFunc = ini.ReadKey(section, "position_open_func", false);
	settings.positionCloseFunc = ini.ReadKey(section, "position_close_func", false);

	settings.level1DataSource = Settings::MARKET_DATA_SOURCE_IQFEED;
	settings.level2DataSource = m_settings.level2DataSource
		?	m_settings.level2DataSource
		:	Util::ConvertStrToMarketDataSource(ini.ReadKey(section, "level2_data_source", false), true);

	SettingsReport report;
	AppendSettingsReport("name", settings.algoName, report);
	AppendSettingsReport("script_file", settings.scriptFile, report);
	AppendSettingsReport("position_open_func", settings.positionOpenFunc, report);
	AppendSettingsReport("position_close_func", settings.positionCloseFunc, report);
	AppendSettingsReport(
		"level1_data_source",
		Util::ConvertToStr(settings.level1DataSource),
		report);
	AppendSettingsReport(
		"level2_data_source",
		Util::ConvertToStr(settings.level2DataSource),
		report);
	ReportSettings(report);

	m_settings = settings;

	UpdateCallbacks();

}

void PyApi::Algo::UpdateCallbacks() {
	//...//
}
