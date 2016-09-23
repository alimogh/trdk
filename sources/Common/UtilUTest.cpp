/**************************************************************************
 *   Created: 2016/09/10 15:21:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

namespace lib = trdk::Lib;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

TEST(UtilsTest, CovertTimeToFileName) {

	EXPECT_EQ(
		"20010101_000000",
		lib::ConvertToFileName(
			pt::ptime(gr::date(2001, 1, 1), pt::time_duration(0, 0, 0))));
	EXPECT_EQ(
		"20010101_010101",
		lib::ConvertToFileName(
			pt::ptime(gr::date(2001, 1, 1), pt::time_duration(1, 1, 1))));
	EXPECT_EQ(
		"20101112_121314",
		lib::ConvertToFileName(
			pt::ptime(gr::date(2010, 11, 12), pt::time_duration(12, 13, 14))));

	EXPECT_EQ("20010101", lib::ConvertToFileName(gr::date(2001, 1, 1)));
	EXPECT_EQ("20101112", lib::ConvertToFileName(gr::date(2010, 11, 12)));

	EXPECT_EQ("000000", lib::ConvertToFileName(pt::time_duration(0, 0, 0)));
	EXPECT_EQ("010101", lib::ConvertToFileName(pt::time_duration(1, 1, 1)));
	EXPECT_EQ("121314", lib::ConvertToFileName(pt::time_duration(12, 13, 14)));
	
}
