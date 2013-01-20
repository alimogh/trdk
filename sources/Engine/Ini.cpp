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
using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;
using namespace Trader::Engine::Ini;

const std::string Sections::common = "Common";
const std::string Sections::defaults = "Defaults";
const std::string Sections::strategy = "Strategy";
const std::string Sections::tradeSystem = "TradeSystem";
const std::string Sections::observer = "Observer";
const std::string Sections::service = "Service";
const std::string Sections::MarketData::source = "MarketData.Source";

const std::string Sections::MarketData::Log::symbols = "MarketData.Log.Symbols";
const std::string Sections::MarketData::request = "MarketData.Request";

const std::string Keys::module = "module";
const std::string Keys::fabric = "fabric";
const std::string Keys::symbols = "symbols";
const std::string Keys::primaryExchange = "primary_exchange";
const std::string Keys::exchange = "exchange";
const std::string Keys::services = "services";

const std::string Variables::currentSymbol = "CURRENT_SYMBOL";

const std::string Constants::Services::level1 = "Level 1";
const std::string Constants::Services::trades = "Trades";

const std::string DefaultValues::Fabrics::strategy = "CreateStrategy";
const std::string DefaultValues::Fabrics::service = "CreateService";
const std::string DefaultValues::Fabrics::observer = "CreateObserver";

//////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Settings> Ini::LoadSettings(
			const IniFile &ini,
			const boost::posix_time::ptime &now,
			bool isPlayMode) {
	return boost::shared_ptr<Settings>(
		new Settings(ini, Ini::Sections::common, now, isPlayMode));
}
