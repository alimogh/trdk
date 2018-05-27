/**************************************************************************
 *   Created: 2016/11/15 02:33:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Security.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;
using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Test;

namespace {

class TrdkMarketDataLogSource : public Test::MarketDataSource {
 public:
  typedef Test::MarketDataSource Base;

 public:
  explicit TrdkMarketDataLogSource(Context &context,
                                   const std::string &instanceName,
                                   const ptr::ptree &conf)
      : Base(context, instanceName, conf),
        m_filePath(Normalize(conf.get<fs::path>("source"))) {
    GetLog().Info("Source is %1%.", m_filePath);
  }

  ~TrdkMarketDataLogSource() override {
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
    if (!symbol.IsExplicit()) {
      throw Exception("Source works only with explicit symbols");
    } else if (m_security) {
      throw Exception("Source works only with one security");
    }

    auto result = boost::make_shared<Test::Security>(
        GetContext(), symbol, *this, Test::Security::SupportedLevel1Types());
    result->SetTradingSessionState(pt::not_a_date_time, true);
    result->SetOnline(pt::not_a_date_time, true);

    m_security = result;

    return *result;
  }

  virtual void Run() override {
    if (!m_security) {
      throw Exception("Security is not set");
    }

    std::ifstream file(m_filePath.string().c_str());
    if (!file) {
      GetLog().Error("Failed to open market data source file %1%.", m_filePath);
      throw ConnectError("Failed to open market data source file");
    } else {
      GetLog().Info("Opened market data source file %1%.", m_filePath);
    }

    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line)) {
      if (IsStopped()) {
        GetLog().Info("Stopped.");
        break;
      }

      ++lineNo;

      std::vector<std::string> fields;
      boost::split(fields, line, boost::is_any_of(","),
                   boost::token_compress_on);

      const bool isBreakPoint = fields[0] == "b";
      if (isBreakPoint) {
        fields.erase(fields.begin());
      }

      if (fields.size() < 3) {
        if (boost::starts_with(line, "[Start]")) {
          GetLog().Info("Found session end on line %1%.", lineNo);
          break;
        }
        GetLog().Error("Wrong file format at line %1%.", lineNo);
        throw Exception("Wrong file format");
      }

      const auto recordTime = pt::time_from_string(fields[0]);
      if (isBreakPoint) {
        GetLog().Debug("Breakpoint found on line %1%. Record time: %2%.",
                       lineNo, recordTime);
      }

      GetContext().SetCurrentTime(recordTime, true);

      const pt::ptime dataTime = fields[1] == "not-a-date-time"
                                     ? pt::not_a_date_time
                                     : pt::time_from_string(fields[1]);

      if (fields[2] == "T") {
        OnTick(dataTime, fields);
      } else if (fields[2] == "L1U") {
        OnLevel1Update(dataTime, fields);
      } else {
        throw Exception("Unknown data record type");
      }

      GetContext().SyncDispatching();
    }

    GetLog().Info("Read %1% lines from %2%.", lineNo, m_filePath);
  }

 protected:
  void OnTick(const pt::ptime &time, const std::vector<std::string> &fields) {
    if (fields.size() != 7 && fields.size() != 6) {
      throw Exception("Tick record has wrong number of fields");
    }
    m_security->AddTrade(time, boost::lexical_cast<double>(fields[3]),
                         boost::lexical_cast<double>(fields[4]),
                         TimeMeasurement::Milestones(),
                         boost::lexical_cast<bool>(fields[5]));
  }

  void OnLevel1Update(const pt::ptime &time,
                      const std::vector<std::string> &fields) {
    if (fields.size() < 4 || (fields.size() - 3) % 2) {
      throw Exception("Level 1 Update record has wrong format");
    }

    std::vector<Level1TickValue> update;
    for (auto it = fields.cbegin() + 3; it != fields.cend();
         std::advance(it, 2)) {
      update.emplace_back(ConvertToLevel1TickType(*it),
                          boost::lexical_cast<double>(*std::next(it)));
    }

    m_security->SetLevel1(time, update, TimeMeasurement::Milestones());
  }

 private:
  const boost::filesystem::path m_filePath;
  boost::shared_ptr<Test::Security> m_security;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::MarketDataSource> CreateTrdkMarketDataLogSource(
    Context &context,
    const std::string &instanceName,
    const ptr::ptree &configuration) {
  return boost::make_shared<TrdkMarketDataLogSource>(context, instanceName,
                                                     configuration);
}

////////////////////////////////////////////////////////////////////////////////
