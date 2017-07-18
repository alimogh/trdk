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
  EXPECT_EQ("20010101_000000",
            lib::ConvertToFileName(
                pt::ptime(gr::date(2001, 1, 1), pt::time_duration(0, 0, 0))));
  EXPECT_EQ("20010101_010101",
            lib::ConvertToFileName(
                pt::ptime(gr::date(2001, 1, 1), pt::time_duration(1, 1, 1))));
  EXPECT_EQ("20101112_121314",
            lib::ConvertToFileName(pt::ptime(gr::date(2010, 11, 12),
                                             pt::time_duration(12, 13, 14))));

  EXPECT_EQ("20010101", lib::ConvertToFileName(gr::date(2001, 1, 1)));
  EXPECT_EQ("20101112", lib::ConvertToFileName(gr::date(2010, 11, 12)));

  EXPECT_EQ("000000", lib::ConvertToFileName(pt::time_duration(0, 0, 0)));
  EXPECT_EQ("010101", lib::ConvertToFileName(pt::time_duration(1, 1, 1)));
  EXPECT_EQ("121314", lib::ConvertToFileName(pt::time_duration(12, 13, 14)));
}

TEST(UtilsTest, GetTimeByTimeOfDayAndDate) {
  // TZ, control time from prev day, source from next day
  EXPECT_EQ(pt::ptime(gr::date(2016, 12, 7), pt::time_duration(1, 10, 30)),
            lib::GetTimeByTimeOfDayAndDate(
                pt::time_duration(2, 10, 30),
                pt::ptime(gr::date(2016, 12, 6), pt::time_duration(23, 03, 56)),
                pt::time_duration(1, 0, 0)));

  // TZ, control time from next day, source from prev day
  EXPECT_EQ(pt::ptime(gr::date(2016, 12, 6), pt::time_duration(21, 10, 30)),
            lib::GetTimeByTimeOfDayAndDate(
                pt::time_duration(22, 10, 30),
                pt::ptime(gr::date(2016, 12, 7), pt::time_duration(1, 10, 36)),
                pt::time_duration(1, 0, 0)));

  // TZ, control time and source from one day, source after control
  EXPECT_EQ(pt::ptime(gr::date(2016, 12, 6), pt::time_duration(21, 10, 30)),
            lib::GetTimeByTimeOfDayAndDate(
                pt::time_duration(22, 10, 30),
                pt::ptime(gr::date(2016, 12, 6), pt::time_duration(21, 03, 56)),
                pt::time_duration(1, 0, 0)));

  // TZ, control time and source from one day, source before control
  EXPECT_EQ(pt::ptime(gr::date(2016, 12, 6), pt::time_duration(21, 10, 30)),
            lib::GetTimeByTimeOfDayAndDate(
                pt::time_duration(22, 10, 30),
                pt::ptime(gr::date(2016, 12, 7), pt::time_duration(3, 03, 56)),
                pt::time_duration(1, 0, 0)));
}
