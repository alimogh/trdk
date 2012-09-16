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

namespace fs = boost::filesystem;
using namespace Ini;
using namespace Trader;

const std::string Sections::common = "Common";
const std::string Sections::algo = "Algo.";
const std::string Sections::tradeSystem = "TradeSystem";
const std::string Sections::MarketData::Source::live = "MarketData.Source.Live";

const std::string Sections::MarketData::Log::symbols = "MarketData.Log.Symbols";
const std::string Sections::MarketData::request = "MarketData.Request";

const std::string Key::module = "module";
const std::string Key::fabric = "fabric";
const std::string Key::symbols = "symbols";

//////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Settings> Ini::LoadSettings(
			const IniFile &ini,
			const boost::posix_time::ptime &now,
			bool isPlayMode) {
	return boost::shared_ptr<Settings>(
		new Settings(ini, Ini::Sections::common, now, isPlayMode));
}
