/**************************************************************************
 *   Created: 2017/01/09 00:15:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SourcesSynchronizer.hpp"
#include "Tests/MockService.hpp"

namespace lib = trdk::Lib;
namespace tl = trdk::TradingLib;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

using namespace testing;
using namespace trdk::Tests;

TEST(SourcesSynchronizer, OneStartedEarly2) {

	MockService service1;
	MockService service2;

	tl::SourcesSynchronizer sync;
	sync.Add(service1);
	sync.Add(service2);
	ASSERT_EQ(2, sync.GetSize());

	pt::ptime service1Time(
		gr::date(2017, 01, 9),
		pt::time_duration(0, 25, 43));

	EXPECT_CALL(service1, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service1Time));
	for (size_t i = 0; i < 5; ++i) {
		EXPECT_FALSE(sync.Sync(service1)) << i;
		service1Time += pt::minutes(5);
	}
	
	auto service2Time = service1Time;
	EXPECT_CALL(service2, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service2Time));
	EXPECT_FALSE(sync.Sync(service2));
	EXPECT_TRUE(sync.Sync(service1));

	for (size_t i = 0; i < 100; ++i) {
		service1Time += pt::minutes(5);
		service2Time = service1Time;
		EXPECT_FALSE(sync.Sync(service1));
		EXPECT_TRUE(sync.Sync(service2));
	}

	for (size_t i = 0; i < 100; ++i) {
		service1Time += pt::minutes(5);
		service2Time = service1Time;
		EXPECT_FALSE(sync.Sync(service2));
		EXPECT_TRUE(sync.Sync(service1));
	}

}

TEST(SourcesSynchronizer, OneStartedEarly3) {

	MockService service1;
	MockService service2;
	MockService service3;

	tl::SourcesSynchronizer sync;
	sync.Add(service1);
	sync.Add(service2);
	sync.Add(service3);
	ASSERT_EQ(3, sync.GetSize());

	pt::ptime service1Time(
		gr::date(2017, 01, 9),
		pt::time_duration(0, 25, 43));

	EXPECT_CALL(service1, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service1Time));
	for (size_t i = 0; i < 5; ++i) {
		EXPECT_FALSE(sync.Sync(service1)) << i;
		service1Time += pt::minutes(5);
	}
	
	auto service2Time = service1Time;
	EXPECT_CALL(service2, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service2Time));
	EXPECT_FALSE(sync.Sync(service2));
	EXPECT_FALSE(sync.Sync(service1));

	for (size_t i = 0; i < 5; ++i) {
		EXPECT_FALSE(sync.Sync(service1)) << i;
		EXPECT_FALSE(sync.Sync(service2)) << i;
		service1Time += pt::minutes(5);
		service2Time = service1Time;
	}

	auto service3Time = service1Time;
	EXPECT_CALL(service3, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service3Time));
	EXPECT_FALSE(sync.Sync(service3));
	EXPECT_FALSE(sync.Sync(service1));
	EXPECT_TRUE(sync.Sync(service2));

	for (size_t i = 0; i < 100; ++i) {
		service1Time += pt::minutes(5);
		service2Time
			= service3Time
			= service1Time;
		EXPECT_FALSE(sync.Sync(service3));
		EXPECT_FALSE(sync.Sync(service1));
		EXPECT_TRUE(sync.Sync(service2));
	}

	for (size_t i = 0; i < 100; ++i) {
		service1Time += pt::minutes(5);
		service2Time
			= service3Time
			= service1Time;
		EXPECT_FALSE(sync.Sync(service1));
		EXPECT_FALSE(sync.Sync(service3));
		EXPECT_TRUE(sync.Sync(service2));
	}

	for (size_t i = 0; i < 100; ++i) {
		service1Time += pt::minutes(5);
		service2Time
			= service3Time
			= service1Time;
		EXPECT_FALSE(sync.Sync(service1));
		EXPECT_FALSE(sync.Sync(service2));
		EXPECT_TRUE(sync.Sync(service3));
	}

}

TEST(SourcesSynchronizer, LostUpdate) {

	MockService service1;
	MockService service2;

	tl::SourcesSynchronizer sync;
	sync.Add(service1);
	sync.Add(service2);
	ASSERT_EQ(2, sync.GetSize());

	pt::ptime service1Time(
		gr::date(2017, 01, 9),
		pt::time_duration(0, 25, 43));

	EXPECT_CALL(service1, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service1Time));
	EXPECT_FALSE(sync.Sync(service1));

	service1Time += pt::minutes(5);
	auto service2Time = service1Time;
	EXPECT_CALL(service2, GetLastDataTime())
		.WillRepeatedly(ReturnRef(service2Time));
	EXPECT_FALSE(sync.Sync(service2));
	EXPECT_TRUE(sync.Sync(service1));

	service1Time += pt::minutes(5);
	service2Time = service1Time;
	EXPECT_FALSE(sync.Sync(service1));
	EXPECT_THROW(sync.Sync(service1), trdk::Lib::Exception);

	service1Time += pt::minutes(5);
	EXPECT_THROW(sync.Sync(service1), trdk::Lib::Exception);

	EXPECT_TRUE(sync.Sync(service2));
	EXPECT_FALSE(sync.Sync(service1));

}

TEST(SourcesSynchronizer, UnknownService) {

	MockService service1;
	MockService service2;
	MockService service3;

	tl::SourcesSynchronizer sync;
	sync.Add(service1);
	sync.Add(service2);
	ASSERT_EQ(2, sync.GetSize());

	pt::ptime time(gr::date(2017, 01, 9), pt::time_duration(0, 25, 43));
	EXPECT_CALL(service1, GetLastDataTime()).WillRepeatedly(ReturnRef(time));
	EXPECT_CALL(service2, GetLastDataTime()).WillRepeatedly(ReturnRef(time));
	EXPECT_CALL(service3, GetLastDataTime()).Times(0);

	EXPECT_FALSE(sync.Sync(service3));

	time += pt::minutes(5);
	EXPECT_FALSE(sync.Sync(service2));
	EXPECT_TRUE(sync.Sync(service1));
	EXPECT_FALSE(sync.Sync(service3));

	time += pt::minutes(5);
	EXPECT_FALSE(sync.Sync(service2));
	EXPECT_FALSE(sync.Sync(service3));
	EXPECT_TRUE(sync.Sync(service1));

	time += pt::minutes(5);
	EXPECT_FALSE(sync.Sync(service3));
	EXPECT_FALSE(sync.Sync(service1));
	EXPECT_TRUE(sync.Sync(service2));

}
