/**************************************************************************
 *   Created: 2012/6/14/ 20:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include "Util.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/thread/thread_time.hpp>
#	include <boost/algorithm/string.hpp>
#include "DisableBoostWarningsEnd.h"
#include "Assert.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace lt = boost::local_time;

fs::path Util::SymbolToFilePath(
			const boost::filesystem::path &path,
			const std::string &symbol,
			const std::string &ext) {
	std::list<std::string> subs;
	boost::split(subs, symbol, boost::is_any_of(":"));
	fs::path result;
	result /= path;
	result /= boost::join(subs, "_");
	result.replace_extension((boost::format(".%1%") % ext).str());
	return result;
}

namespace {
	const lt::posix_time_zone edtTimeZone(
		"EST-05EDT+01,M4.1.0/02:00,M10.5.0/02:00");
}

boost::shared_ptr<lt::posix_time_zone> Util::GetEdtTimeZone() {
	return boost::shared_ptr<lt::posix_time_zone>(
		new  lt::posix_time_zone(edtTimeZone));
}

pt::time_duration Util::GetEdtDiff() {
	const lt::local_date_time estTime(boost::get_system_time(), GetEdtTimeZone());
	return estTime.local_time() - estTime.utc_time();
}

std::string Util::CreateSymbolFullStr(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange) {
	return (boost::format("%1%:%2%:%3%") % symbol % primaryExchange % exchange).str();
}

namespace {
	const pt::ptime unixEpochStart(boost::gregorian::date(1970, 1, 1));
}

time_t Util::ConvertToTimeT(const pt::ptime &source) {
	Assert(!source.is_special());
	AssertGe(source, unixEpochStart);
	if (source < unixEpochStart) {
		return 0;
	}
	const pt::time_duration durationFromTEpoch(source - unixEpochStart);
	return static_cast<time_t>(durationFromTEpoch.total_seconds());
}
