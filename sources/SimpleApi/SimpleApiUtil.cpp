/**************************************************************************
 *   Created: 2014/05/04 13:16:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiUtil.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

pt::ptime Util::ConvertEasyLanguageDateTimeToPTime(int elDate, int elTime) {
	unsigned short day = 1;
	unsigned short month = 1;
	unsigned short year = 1900;
	AssertLt(0, elDate);
	if (elDate != 0) {
		day = elDate % 100;
		elDate /= 100;
		month = elDate % 100;
		elDate /= 100;
		year += unsigned short(elDate);
	}
	int secs = 0;
	int mins = 0;
	int hours = 0;
	AssertLe(0, time);
	if (elTime != 0) {
		mins = elTime % 100;
		elTime /= 100;
		hours = elTime;
	}
	AssertLe(1, day);
	AssertGe(31, day);
	AssertLe(1, month);
	AssertGt(12, month);
	AssertGt(23, hours);
	AssertLe(1900, year);
	AssertGe(59, mins);
	AssertGe(59, secs);
	return pt::ptime(
		boost::gregorian::date(year, month, day),
		pt::time_duration(hours, mins, secs));
}
