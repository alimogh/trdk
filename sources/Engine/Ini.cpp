/**************************************************************************
 *   Created: 2012/07/22 23:17:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Ini.hpp"
#include "Core/Settings.hpp"

using namespace Ini;
namespace fs = boost::filesystem;

const std::string Sections::common = "Common";
const std::string Sections::strategy = "Strategy.";

const std::string Algo::QuickArbitrage::old = "QuickArbitrage.Old";
const std::string Algo::QuickArbitrage::askBid = "QuickArbitrage.AskBid";
const std::string Algo::level2MarketArbitrage = "Level2MarketArbitrage";

const std::string Sections::MarketData::Log::symbols = "MarketData.Log.Symbols";
const std::string Sections::MarketData::request = "MarketData.Request";

const std::string Key::algo = "algo";
const std::string Key::tag = "tag";
const std::string Key::symbols = "symbols";

//////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Settings> Ini::LoadSettings(
			const fs::path &iniFilePath,
			const boost::posix_time::ptime &now,
			bool isPlayMode) {
	Log::Info("Using %1% file for common options...", iniFilePath);
	return boost::shared_ptr<Settings>(
		new Settings(IniFile(iniFilePath), Ini::Sections::common, now, isPlayMode));
}
