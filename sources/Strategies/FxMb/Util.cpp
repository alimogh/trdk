/**************************************************************************
 *   Created: 2014/10/22 15:59:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies;

namespace pt = boost::posix_time;

pt::time_duration FxMb::ReadPositionGracePeriod(
			const IniSectionRef &conf,
			const char *key) {
	const auto value = conf.GetBase().ReadTypedKey<double>("Common", key);
	return pt::milliseconds(size_t(value * 1000));
}
