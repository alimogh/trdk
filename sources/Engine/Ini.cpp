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

const std::string Sections::strategy = "Strategy";
const std::string Sections::observer = "Observer";
const std::string Sections::service = "Service";

const std::string Keys::module = "module";
const std::string Keys::factory = "factory";
const std::string Keys::instances = "instances";
const std::string Keys::requires = "requires";

const std::string Constants::Services::level1Updates = "Level 1 updates";
const std::string Constants::Services::level1Ticks = "Level 1 ticks";
const std::string Constants::Services::trades = "Trades";
const std::string Constants::Services::brokerPositionsUpdates
	= "Broker positions";

const std::string DefaultValues::Modules::service = "Services";
