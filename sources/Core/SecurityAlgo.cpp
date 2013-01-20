/**************************************************************************
 *   Created: 2012/11/04 15:48:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgo.hpp"
#include "Security.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using namespace Trader;
using namespace Trader::Lib;

SecurityAlgo::SecurityAlgo(
			const std::string &typeName,
			const std::string &name,
			const std::string &tag,
			boost::shared_ptr<const Settings> settings)
		: Module(typeName, name, tag, settings) {
	//...//
}

SecurityAlgo::~SecurityAlgo() {
	//...//
}

pt::ptime SecurityAlgo::GetCurrentTime() const {
	const auto &security = GetSecurity();
	return security.GetSettings().IsReplayMode()
		?	boost::get_system_time()
		:	security.GetLastMarketDataTime();
}
