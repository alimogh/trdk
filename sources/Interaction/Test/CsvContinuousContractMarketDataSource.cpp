/**************************************************************************
 *   Created: 2016/09/29 03:44:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;
using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Test;

namespace {

class CsvContinuousContractMarketDataSource : public Test::MarketDataSource {
 public:
  typedef Test::MarketDataSource Base;

 public:
  explicit CsvContinuousContractMarketDataSource(Context &context,
                                                 std::string instanceName,
                                                 std::string title,
                                                 const ptr::ptree &conf)
      : Base(context, std::move(instanceName), std::move(title), conf) {}

  virtual ~CsvContinuousContractMarketDataSource() override {
    try {
      Stop();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

 protected:
  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    if (symbol.IsExplicit()) {
      throw Exception("Source works only with not explicit symbols");
    } else if (m_security) {
      throw Exception("Source works only with one security");
    }

    auto result = boost::make_shared<Test::Security>(
        GetContext(), symbol, *this, Test::Security::SupportedLevel1Types());

    result->SetTradingSessionState(pt::not_a_date_time, true);
    m_security = result;

    return *result;
  }

  virtual void Run() override {
    size_t fileNo = 1;
    pt::ptime currentTime;
    pt::ptime requestTime;
    while (ReadFile(fileNo, currentTime, requestTime)) {
      ++fileNo;
    }
    GetLog().Info("Read %1% files. Stopped at %2%.", fileNo - 1, currentTime);
  }

 private:
  bool ReadFile(size_t fileNo, pt::ptime &currentTime, pt::ptime &requestTime) {
    const std::string fileName =
        (boost::format("MarketData_%1%.csv") % fileNo).str();

    std::ifstream file(fileName.c_str());
    if (!file) {
      GetLog().Info("Can not open \"%1%\".", fileName);
      return false;
    } else {
      GetLog().Info("Opened \"%1%\".", fileName);
    }

    const pt::time_duration sessionOpeningTime(17, 0, 0);
    std::vector<std::string> fields;
    std::string line;
    size_t lineNo = 0;
    pt::ptime prevTime;
    while (std::getline(file, line)) {
      if (IsStopped()) {
        GetLog().Info("Stopped.");
        break;
      }

      ++lineNo;

      boost::split(fields, line, boost::is_any_of(","),
                   boost::token_compress_on);
      if (fields.size() < 6) {
        GetLog().Error("Wrong file format at line %1%.", lineNo);
        throw Exception("Wrong file format");
      }

      if (lineNo == 1) {
        // Head.
        continue;
      }

      std::vector<std::string> dateField;
      boost::split(dateField, fields[0], boost::is_any_of("/"),
                   boost::token_compress_on);
      if (dateField.size() != 3) {
        GetLog().Error("Wrong date field format at line %1%.", lineNo);
        throw Exception("Wrong file format");
      }

      const pt::ptime time(gr::from_string(dateField[2] + "/" + dateField[0] +
                                           "/" + dateField[1]),
                           pt::duration_from_string(fields[1]));
      if (!requestTime.is_not_a_date_time()) {
        if (time == requestTime) {
          requestTime = pt::not_a_date_time;
        } else if (time < requestTime) {
          prevTime = time;
          continue;
        } else if (!prevTime.is_not_a_date_time()) {
          AssertLt(prevTime, requestTime);
          return ReadFile(fileNo, currentTime, prevTime);
        }
      }

      const double v1 = boost::lexical_cast<double>(fields[2]);
      const double v2 = boost::lexical_cast<double>(fields[3]);
      const double v3 = boost::lexical_cast<double>(fields[4]);
      const double v4 = boost::lexical_cast<double>(fields[5]);
      fields.clear();

      const auto expiration = GetContext().GetExpirationCalendar().Find(
          m_security->GetSymbol(), time, sessionOpeningTime);
      if (!expiration) {
        boost::format error(
            "Failed to find expiration info for \"%1%\" and %2%");
        error % *m_security % time;
        throw Exception(error.str().c_str());
      }
      if (!m_security->HasExpiration()) {
        AssertEq(currentTime, pt::not_a_date_time);
        GetContext().SetCurrentTime(time, true);
        m_security->SetOnline(time, true);
        Verify(!m_security->SetExpiration(time, *expiration));
        currentTime = time;
      } else if (*expiration > m_security->GetExpiration()) {
        GetLog().Info("Detected %1% expiration at %2% in \"%3%\".", *m_security,
                      prevTime, fileName);
        Assert(m_security->IsOnline());
        Assert(!prevTime.is_not_a_date_time());
        AssertEq(prevTime, GetContext().GetCurrentTime());
        currentTime = prevTime;
        currentTime += pt::hours(1);
        currentTime = pt::ptime(
            currentTime.date(),
            pt::time_duration(currentTime.time_of_day().hours(), 0, 0));
        if (m_security->SetExpiration(currentTime, *expiration)) {
          requestTime = m_security->GetRequest().GetTime();
          AssertLe(requestTime, GetContext().GetCurrentTime());
          m_security->SetOnline(GetContext().GetCurrentTime(), false);
        } else {
          Assert(m_security->IsOnline());
          requestTime = currentTime;
        }
        break;
      } else {
        if (!m_security->IsOnline() && currentTime <= time) {
          m_security->SetOnline(time, true);
        }
        if (m_security->IsOnline()) {
          currentTime = time;
          GetContext().SetCurrentTime(time, true);
        }
      }

#if 1
      {
        if (m_security->IsOnline()) {
          m_security->SetLevel1(
              time, Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(v4 - 0.01),
              Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(1),
              Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(v4 + 0.01),
              Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(1),
              TimeMeasurement::Milestones());
        }
      }
#endif

#if 1
      {
        m_security->AddTrade(time, v1, 1, TimeMeasurement::Milestones(),
                             m_security->IsOnline());
        m_security->AddTrade(time, v2, 1, TimeMeasurement::Milestones(),
                             m_security->IsOnline());
        m_security->AddTrade(time, v3, 1, TimeMeasurement::Milestones(),
                             m_security->IsOnline());
        m_security->AddTrade(time, v4, 1, TimeMeasurement::Milestones(),
                             m_security->IsOnline());
      }
#endif

      if (m_security->IsOnline()) {
        GetContext().SyncDispatching();
      }

      prevTime = time;
    }

    GetLog().Info("Read %1% lines from \"%2%\".", lineNo, fileName);

    return lineNo > 0;
  }

 private:
  boost::shared_ptr<Test::Security> m_security;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::MarketDataSource>
CreateCsvContinuousContractMarketDataSource(Context &context,
                                            std::string instanceName,
                                            std::string title,
                                            const ptr::ptree &configuration) {
  return boost::make_shared<CsvContinuousContractMarketDataSource>(
      context, std::move(instanceName), std::move(title), configuration);
}

////////////////////////////////////////////////////////////////////////////////
