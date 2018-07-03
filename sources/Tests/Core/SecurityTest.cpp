/**************************************************************************
 *   Created: 2016/09/18 14:14:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Prec.hpp"
#include "Core/Security.hpp"

using namespace testing;

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

namespace Core {

TEST(Security, Request) {
  using Request = trdk::Security::Request;

  {
    Request request;
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(0, request.GetNumberOfTicks());
  }

  {
    Request request;

    request.RequestNumberOfTicks(10);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(10, request.GetNumberOfTicks());

    request.RequestNumberOfTicks(5);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(10, request.GetNumberOfTicks());

    request.RequestNumberOfTicks(15);
    EXPECT_EQ(request.GetTime(), pt::not_a_date_time);
    EXPECT_EQ(15, request.GetNumberOfTicks());
  }

  {
    Request request;

    const auto &time1 =
        pt::ptime(gr::date(1983, 5, 23), pt::time_duration(7, 15, 14));
    request.RequestTime(time1);
    EXPECT_EQ(time1, request.GetTime());
    EXPECT_EQ(0, request.GetNumberOfTicks());

    const auto &time2 =
        pt::ptime(gr::date(2009, 1, 7), pt::time_duration(16, 15, 14));
    request.RequestTime(time2);
    EXPECT_EQ(time1, request.GetTime());
    EXPECT_EQ(0, request.GetNumberOfTicks());

    const auto &time3 =
        pt::ptime(gr::date(1980, 6, 4), pt::time_duration(6, 15, 14));
    request.RequestTime(time3);
    EXPECT_EQ(time3, request.GetTime());
    EXPECT_EQ(0, request.GetNumberOfTicks());
  }

  {
    Request request1;
    const auto &time1 =
        pt::ptime(gr::date(1983, 5, 23), pt::time_duration(7, 15, 14));
    request1.RequestTime(time1);
    request1.RequestNumberOfTicks(10);

    Request request2;
    const auto &time2 =
        pt::ptime(gr::date(1980, 6, 4), pt::time_duration(6, 15, 14));
    request2.RequestTime(time2);
    request2.RequestNumberOfTicks(5);

    Request request3(request1);
    request3.Merge(request2);
    EXPECT_EQ(time2, request3.GetTime());
    EXPECT_EQ(10, request3.GetNumberOfTicks());
    EXPECT_EQ(time2, request2.GetTime());
    EXPECT_EQ(5, request2.GetNumberOfTicks());

    Request request4(request2);
    request4.Merge(request1);
    EXPECT_EQ(time2, request4.GetTime());
    EXPECT_EQ(10, request4.GetNumberOfTicks());
    EXPECT_EQ(time1, request1.GetTime());
    EXPECT_EQ(10, request1.GetNumberOfTicks());
  }

  {
    Request request1;
    const auto &time1 =
        pt::ptime(gr::date(1983, 5, 23), pt::time_duration(7, 15, 14));
    request1.RequestTime(time1);
    request1.RequestNumberOfTicks(10);

    Request request2;
    const auto &time2 =
        pt::ptime(gr::date(1980, 6, 4), pt::time_duration(6, 15, 14));
    request2.RequestTime(time2);
    request2.RequestNumberOfTicks(5);

    request1.Swap(request2);
    EXPECT_EQ(time2, request1.GetTime());
    EXPECT_EQ(5, request1.GetNumberOfTicks());
    EXPECT_EQ(time1, request2.GetTime());
    EXPECT_EQ(10, request2.GetNumberOfTicks());

    request2.Swap(request1);
    EXPECT_EQ(time1, request1.GetTime());
    EXPECT_EQ(10, request1.GetNumberOfTicks());
    EXPECT_EQ(time2, request2.GetTime());
    EXPECT_EQ(5, request2.GetNumberOfTicks());
  }

  {
    Request request1;
    const auto &time1 =
        pt::ptime(gr::date(1983, 5, 23), pt::time_duration(7, 15, 14));
    request1.RequestTime(time1);
    request1.RequestNumberOfTicks(10);

    Request request2;
    const auto &time2 =
        pt::ptime(gr::date(1980, 6, 4), pt::time_duration(6, 15, 14));
    request2.RequestTime(time2);
    request2.RequestNumberOfTicks(5);

    EXPECT_TRUE(request1.IsEarlier(request2));
    EXPECT_TRUE(request2.IsEarlier(request1));
    EXPECT_FALSE(request1.IsEarlier(request1));
    EXPECT_FALSE(request2.IsEarlier(request2));

    Request request3;
    request3.RequestNumberOfTicks(5);
    EXPECT_TRUE(request1.IsEarlier(request3));
    EXPECT_FALSE(request3.IsEarlier(request1));
    request3.RequestNumberOfTicks(15);
    EXPECT_TRUE(request1.IsEarlier(request3));
    EXPECT_TRUE(request3.IsEarlier(request1));

    Request request4;
    const auto &time4 =
        pt::ptime(gr::date(1985, 5, 23), pt::time_duration(7, 15, 14));
    request4.RequestTime(time4);
    EXPECT_TRUE(request1.IsEarlier(request4));
    EXPECT_FALSE(request4.IsEarlier(request1));
    request4.RequestTime(time2);
    EXPECT_TRUE(request1.IsEarlier(request4));
    EXPECT_TRUE(request4.IsEarlier(request1));

    EXPECT_TRUE(request3.IsEarlier(request4));
    EXPECT_TRUE(request4.IsEarlier(request3));
  }
}

}  // namespace Core