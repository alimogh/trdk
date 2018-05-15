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

namespace fs = boost::filesystem;
using namespace trdk;
using namespace Lib;
using namespace Engine;
using namespace Ini;

const std::string Sections::strategy = "Strategy";

const std::string Keys::module = "module";
const std::string Keys::factory = "factory";
const std::string Keys::instances = "instances";
const std::string Keys::requires = "requires";

const std::string Constants::Services::level1Updates = "Level 1 updates";
const std::string Constants::Services::level1Ticks = "Level 1 ticks";
const std::string Constants::Services::trades = "Trades";
const std::string Constants::Services::brokerPositionsUpdates =
    "Broker positions";