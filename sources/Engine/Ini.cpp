/**************************************************************************
 *   Created: 2012/07/22 23:17:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Ini.hpp"
#include "Core/Settings.hpp"

namespace fs = boost::filesystem;
using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;
using namespace trdk::Engine::Ini;

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
const std::string Keys::uses = "uses";

const std::string Constants::Services::level1 = "Level 1";
const std::string Constants::Services::trades = "Trades";

const std::string DefaultValues::Fabrics::strategy = "CreateStrategy";
const std::string DefaultValues::Fabrics::service = "CreateService";
const std::string DefaultValues::Fabrics::observer = "CreateObserver";

const std::string DefaultValues::Modules::service = "Services";
