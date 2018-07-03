/**************************************************************************
 *   Created: 2016/05/02 13:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace lib = trdk::Lib;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

namespace {

fs::path CreateCsvOkSource() {
  const fs::path filePath =
      lib::GetExeWorkingDir() / "ExpirationCalendarOkTestSource.csv";
  std::ofstream file(filePath.string().c_str(), std::ios::trunc);
  if (!file) {
    throw std::exception("Filed to open CSV file for test source");
  }

  std::vector<boost::tuple<const char *, int>> source = {

      {"XXX1", 20160520},

      {"XXX2", 20151110}, {"XXX2", 20151225}, {"XXX2", 20160118},
      {"XXX2", 20160219}, {"XXX2", 20160317}, {"XXX2", 20160420},
      {"XXX2", 20160521},

      {"XXX3", 20160420},

  };
  std::random_shuffle(source.begin(), source.end());

  for (const auto &line : source) {
    file << boost::get<0>(line) << ',' << boost::get<1>(line) << std::endl;
  }

  return filePath;
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace Lib {

class ExpirationCalendar : public testing::Test {
 protected:
  typedef lib::ExpirationCalendar Calendar;
  typedef lib::ContractExpiration Expiration;

 public:
  static void SetUpTestCase() { m_csvSourceFilePath = CreateCsvOkSource(); }

 protected:
  static fs::path m_csvSourceFilePath;
};

fs::path ExpirationCalendar::m_csvSourceFilePath;

TEST_F(ExpirationCalendar, FindByDate) {
  Calendar calendar;
  calendar.ReloadCsv(m_csvSourceFilePath);

  // {"XXX1", 20160520},
  {
    auto it = calendar.Find(lib::Symbol("XXX1/USD::Futures"),
                            gr::date(2016, gr::Apr, 6));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_JUNE, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::May, 20), it->GetDate());
    EXPECT_EQ(std::string("M16"), it->GetContract(false));
    EXPECT_EQ(std::string("M6"), it->GetContract(true));
    EXPECT_FALSE(++it);
  }
  {
    auto it = calendar.Find(lib::Symbol("XXX1/USD::Futures"),
                            gr::date(2016, gr::May, 20));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_JUNE, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::May, 20), it->GetDate());
    EXPECT_EQ(std::string("M16"), it->GetContract(false));
    EXPECT_EQ(std::string("M6"), it->GetContract(true));
    EXPECT_FALSE(++it);
  }

  // {"XXX2", 20151110},
  {
    auto it = calendar.Find(lib::Symbol("XXX2/USD::Futures"),
                            gr::date(2015, gr::Sep, 25));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_DECEMBER, it->GetCode());
    EXPECT_EQ(2015, it->GetYear());
    EXPECT_EQ(gr::date(2015, gr::Nov, 10), it->GetDate());
    EXPECT_EQ(std::string("Z15"), it->GetContract(false));
    EXPECT_EQ(std::string("Z5"), it->GetContract(true));
    EXPECT_TRUE(++it);
  }

  {
    EXPECT_FALSE(calendar.Find(lib::Symbol("XXX2/USD::Futures"),
                               gr::date(2016, gr::May, 22)));
  }

  // {"XXX2", 20160219},
  {
    auto it = calendar.Find(lib::Symbol("XXX2/USD::Futures"),
                            gr::date(2016, gr::Feb, 1));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_MARCH, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::Feb, 19), it->GetDate());
    EXPECT_EQ(std::string("H16"), it->GetContract(false));
    EXPECT_EQ(std::string("H6"), it->GetContract(true));
    EXPECT_TRUE(++it);
  }

  {
    // {"XXX2", 20151110},
    auto it = calendar.Find(lib::Symbol("XXX2/USD::Futures"),
                            gr::date(2015, gr::Sep, 26));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_DECEMBER, it->GetCode());
    EXPECT_EQ(2015, it->GetYear());
    EXPECT_EQ(gr::date(2015, gr::Nov, 10), it->GetDate());
    EXPECT_EQ(std::string("Z15"), it->GetContract(false));
    EXPECT_EQ(std::string("Z5"), it->GetContract(true));

    // {"XXX2", 20151225},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_JANUARY, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2015, gr::Dec, 25), it->GetDate());
    EXPECT_EQ(std::string("F16"), it->GetContract(false));
    EXPECT_EQ(std::string("F6"), it->GetContract(true));

    // {"XXX2", 20160118},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_FEBRUARY, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::Jan, 18), it->GetDate());
    EXPECT_EQ(std::string("G16"), it->GetContract(false));
    EXPECT_EQ(std::string("G6"), it->GetContract(true));

    // {"XXX2", 20160219},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_MARCH, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::Feb, 19), it->GetDate());
    EXPECT_EQ(std::string("H16"), it->GetContract(false));
    EXPECT_EQ(std::string("H6"), it->GetContract(true));

    // {"XXX2", 20160317},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_APRIL, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::Mar, 17), it->GetDate());
    EXPECT_EQ(std::string("J16"), it->GetContract(false));
    EXPECT_EQ(std::string("J6"), it->GetContract(true));

    // {"XXX2", 20160420},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_MAY, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::Apr, 20), it->GetDate());
    EXPECT_EQ(std::string("K16"), it->GetContract(false));
    EXPECT_EQ(std::string("K6"), it->GetContract(true));

    // {"XXX2", 20160521},
    ASSERT_TRUE(++it);
    EXPECT_EQ(Expiration::CODE_JUNE, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::May, 21), it->GetDate());
    EXPECT_EQ(std::string("M16"), it->GetContract(false));
    EXPECT_EQ(std::string("M6"), it->GetContract(true));

    ASSERT_FALSE(++it);
  }
}

TEST_F(ExpirationCalendar, FindByTime) {
  Calendar calendar;
  calendar.ReloadCsv(m_csvSourceFilePath);

  {
    auto it = calendar.Find(
        lib::Symbol("XXX2/USD::Futures"),
        pt::ptime(gr::date(2016, gr::Apr, 21), pt::time_duration(17, 0, 0)),
        pt::time_duration(17, 0, 0));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_JUNE, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::May, 21), it->GetDate());
  }

  {
    auto it = calendar.Find(
        lib::Symbol("XXX2/USD::Futures"),
        pt::ptime(gr::date(2016, gr::Apr, 21), pt::time_duration(17, 0, 1)),
        pt::time_duration(17, 0, 0));
    ASSERT_TRUE(it);
    EXPECT_EQ(Expiration::CODE_JUNE, it->GetCode());
    EXPECT_EQ(2016, it->GetYear());
    EXPECT_EQ(gr::date(2016, gr::May, 21), it->GetDate());
  }

  {
    auto it = calendar.Find(
        lib::Symbol("XXX2/USD::Futures"),
        pt::ptime(gr::date(2016, gr::Apr, 21), pt::time_duration(16, 59, 59)),
        pt::time_duration(17, 0, 0));
    EXPECT_EQ(Expiration::CODE_MAY, it->GetCode()) << *it;
    EXPECT_EQ(2016, it->GetYear()) << *it;
    EXPECT_EQ(gr::date(2016, gr::Apr, 20), it->GetDate()) << *it;
  }
}

TEST_F(ExpirationCalendar, WrongInstrumnetType) {
  Calendar calendar;
  calendar.ReloadCsv(m_csvSourceFilePath);

  for (int i = 0; i < lib::SecurityType::_size(); ++i) {
    if (i == lib::SecurityType::Futures
        //! @todo not supported yet
        || i == lib::SecurityType::FuturesOptions
        //! @todo not supported yet
        || i == lib::SecurityType::ForFuturesOptions
        //! @todo not supported yet
        || i == lib::SecurityType::Options
        //! @todo not supported yet
        || i == lib::SecurityType::Crypto) {
      continue;
    }

    const auto &securityType = reinterpret_cast<lib::SecurityType &>(i);

    const lib::Symbol symbol(std::string("XXX1/USD::") +
                             securityType._to_string());
    boost::format error(
        "No info about expiration for \"%1%\" - wrong security type");
    error % symbol;

    try {
      calendar.Find(symbol, gr::date(2016, gr::May, 22));
      EXPECT_TRUE(false) << "Exception expected.";
    } catch (const std::exception &ex) {
      EXPECT_EQ(error.str(), ex.what());
    }
  }
}

TEST_F(ExpirationCalendar, ErrorNoFile) {
  fs::path wrongPath = lib::GetExeWorkingDir() / "NoSuchFile.csv";
  boost::format error("Filed to open CSV-file %1% with expiration info");
  error % wrongPath;
  try {
    Calendar().ReloadCsv(wrongPath);
    EXPECT_TRUE(false) << "Exception expected.";
  } catch (const std::exception &ex) {
    EXPECT_EQ(error.str(), ex.what());
  }
}

TEST_F(ExpirationCalendar, Stat) {
  Calendar calendar;
  calendar.ReloadCsv(m_csvSourceFilePath);
  const Calendar::Stat &stat = calendar.CalcStat();

  EXPECT_EQ(3, stat.numberOfSymbols);
  EXPECT_EQ(9, stat.numberOfExpirations);
}

TEST_F(ExpirationCalendar, Iterator) {
  Calendar calendar;
  calendar.ReloadCsv(m_csvSourceFilePath);

  // {"XXX2", 20151110},
  auto it1 = calendar.Find(lib::Symbol("XXX2/USD::Futures"),
                           gr::date(2015, gr::Sep, 26));
  ASSERT_TRUE(it1);
  EXPECT_EQ(gr::date(2015, gr::Nov, 10), it1->GetDate());

  EXPECT_EQ(gr::date(2015, gr::Dec, 25), std::next(it1)->GetDate());
  EXPECT_EQ(gr::date(2015, gr::Nov, 10), std::prev(std::next(it1))->GetDate());

  auto it2 = ++it1;
  ASSERT_TRUE(it2);
  EXPECT_EQ(gr::date(2015, gr::Dec, 25), it1->GetDate());
  EXPECT_EQ(gr::date(2015, gr::Dec, 25), it2->GetDate());

  ASSERT_TRUE(++it2);
  EXPECT_EQ(gr::date(2015, gr::Dec, 25), it1->GetDate());
  EXPECT_EQ(gr::date(2016, gr::Jan, 18), it2->GetDate());

  {
    auto it3 = it2++;
    EXPECT_EQ(gr::date(2015, gr::Dec, 25), it1->GetDate());
    EXPECT_EQ(gr::date(2016, gr::Feb, 19), it2->GetDate());
    EXPECT_EQ(gr::date(2016, gr::Jan, 18), it3->GetDate());
  }

  {
    auto it4 = --it2;
    EXPECT_EQ(gr::date(2015, gr::Dec, 25), it1->GetDate());
    EXPECT_EQ(gr::date(2016, gr::Jan, 18), it2->GetDate());
    EXPECT_EQ(gr::date(2016, gr::Jan, 18), it4->GetDate());
  }

  {
    auto it5 = it2--;
    EXPECT_EQ(gr::date(2015, gr::Dec, 25), it1->GetDate());
    EXPECT_EQ(gr::date(2015, gr::Dec, 25), it2->GetDate());
    EXPECT_EQ(gr::date(2016, gr::Jan, 18), it5->GetDate());
  }

  {
    ASSERT_TRUE(--it2);
    auto it5 = --it2;
    EXPECT_FALSE(it2);
    EXPECT_FALSE(it5);
  }
}

}  // namespace Lib

////////////////////////////////////////////////////////////////////////////////
