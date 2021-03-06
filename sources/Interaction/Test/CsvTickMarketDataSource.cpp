/**************************************************************************
 *   Created: 2016/10/15 13:51:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Bar.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;
using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Test;

namespace {

class CsvTickMarketDataSource : public Test::MarketDataSource {
 public:
  typedef Test::MarketDataSource Base;

 private:
  struct Settings {
    fs::path filePath;
    std::string delimiter;
    bool skipFirstLine;

    std::vector<Level1TickType> fields;
    size_t dateFieldIndex;
    size_t timeFieldIndex;
    bool hasLastPriceField;

    explicit Settings(const ptr::ptree &conf)
        : filePath(Normalize(conf.get<fs::path>("source"))),
          delimiter(";" /* @todo fixme conf.ReadKey("delimiter") */),
          skipFirstLine(conf.get<bool>("skipFirstLine")),
          hasLastPriceField(false) {
      dateFieldIndex = timeFieldIndex = 0;
      /* @todo fixme
      bool isDateFieldSet = false;
      bool isTimeFieldSet = false;
      bool isValueFieldSet = false;
      size_t fieldIndex = 0;
      for (const auto &field : conf.ReadList("fields", ",", true)) {
        if (boost::iequals(field, "date")) {
          if (isDateFieldSet) {
            throw Error("Date field set two or more times");
          }
          dateFieldIndex = fieldIndex;
          isDateFieldSet = true;
          fields.emplace_back(numberOfLevel1TickTypes);
        } else if (boost::iequals(field, "time")) {
          if (isTimeFieldSet) {
            throw Error("Time field set two or more times");
          }
          timeFieldIndex = fieldIndex;
          isTimeFieldSet = true;
          fields.emplace_back(numberOfLevel1TickTypes);
        } else if (boost::iequals(field, "date time")) {
          if (isDateFieldSet) {
            throw Error("Date field set two or more times");
          }
          if (isTimeFieldSet) {
            throw Error("Time field set two or more times");
          }
          dateFieldIndex = timeFieldIndex = fieldIndex;
          isDateFieldSet = isTimeFieldSet = true;
          fields.emplace_back(numberOfLevel1TickTypes);
        } else if (boost::iequals(field, "bid price")) {
          AddField(LEVEL1_TICK_BID_PRICE, field);
          isValueFieldSet = true;
        } else if (boost::iequals(field, "ask price")) {
          AddField(LEVEL1_TICK_ASK_PRICE, field);
          isValueFieldSet = true;
        } else if (boost::iequals(field, "last price")) {
          AddField(LEVEL1_TICK_LAST_PRICE, field);
          isValueFieldSet = true;
          hasLastPriceField = true;
        } else {
          boost::format message(
              "Unknown field \"%1%\" is the field list, known fields: "
              "\"date\", \"time\", \"bid price\", \"ask price\" and \"last "
              "price\".");
          message % field;
          throw Error(message.str().c_str());
        }
        ++fieldIndex;
      }
      if (!isDateFieldSet) {
        throw Error("Required field \"date\" is not set");
      }
      if (!isTimeFieldSet) {
        throw Error("Required field \"time\" is not set");
      }
      if (!isValueFieldSet) {
        throw Error("Required any value field is not set");
      }
      */
    }

    template <typename Log>
    void Log(Log &log) const {
      log.Info("Source: %1%. Delimiter: \"%2%\". Skip first line: %3%.",
               filePath,                       // 1
               delimiter,                      // 2
               skipFirstLine ? "yes" : "no");  // 3
    }

   private:
    void AddField(const Level1TickType &type, const std::string &field) {
      if (std::find(fields.cbegin(), fields.cend(), type) != fields.cend()) {
        boost::format message("Field \"%1%\" set two or more times.");
        message % field;
        throw Exception(message.str().c_str());
      }
      fields.emplace_back(type);
    }
  };

 public:
  explicit CsvTickMarketDataSource(Context &context,
                                   std::string instanceName,
                                   std::string title,
                                   const ptr::ptree &conf)
      : Base(context, std::move(instanceName), std::move(title), conf),
        m_settings(conf) {
    m_settings.Log(GetLog());
  }

  ~CsvTickMarketDataSource() override {
    try {
      Stop();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

 protected:
  trdk::Security &CreateNewSecurityObject(const Symbol &symbol) override {
    if (!symbol.IsExplicit()) {
      throw Exception("Source works only with explicit symbols");
    }
    if (m_security) {
      throw Exception("Source works only with one security");
    }

    auto result = boost::make_shared<Test::Security>(
        GetContext(), symbol, *this,
        Test::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_ASK_PRICE)
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_LAST_PRICE));
    result->SetTradingSessionState(pt::not_a_date_time, true);
    result->SetOnline(pt::not_a_date_time, true);

    m_security = result;

    return *result;
  }

  void Run() override {
    if (!m_security) {
      throw Exception("Security is not set");
    }

    std::ifstream file(m_settings.filePath.string().c_str());
    if (!file) {
      GetLog().Error("Failed to open market data source file %1%.",
                     m_settings.filePath);
      throw ConnectError("Failed to open market data source file");
    }
    GetLog().Info("Opened market data source file %1%.", m_settings.filePath);

    std::vector<Level1TickValue> values;
    std::vector<std::string> fields;
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line)) {
      if (IsStopped()) {
        GetLog().Info("Stopped.");
        break;
      }

      ++lineNo;

      boost::split(fields, line, boost::is_any_of(m_settings.delimiter),
                   boost::token_compress_on);
      if (fields.size() < /* @todo fixme m_settings.fields.size()*/ 5) {
        GetLog().Error(
            "Wrong file format at line %1%, there must be at least %2% fields "
            "separated by \"%3%\"",
            lineNo,                    // 1
            m_settings.fields.size(),  // 2
            m_settings.delimiter);     // 3
        throw Exception("Wrong file format");
      }

      if (lineNo == 1 && m_settings.skipFirstLine) {
        // Head.
        continue;
      }

      const pt::ptime time(ParseDateField(fields, lineNo),
                           ParseTimeField(fields, lineNo));

      // @todo fixme size_t fieldIndex = 0;
      values.clear();
      /* @todo fixme
        for (const auto &field : fields) {
        if (fieldIndex >= m_settings.fields.size()) {
          AssertEq(m_settings.fields.size(), fieldIndex);
          break;
        }
        const auto &type = m_settings.fields[fieldIndex++];
        if (type == numberOfLevel1TickTypes) {
          continue;
        }
        values.emplace_back(
            Level1TickValue::Create(type, ParsePriceField(field, lineNo)));
      }
      AssertEq(fields.size(), values.size() + 2);*/
      Bar bar{time,
              boost::none,
              boost::none,
              ParsePriceField(fields[1], lineNo),
              ParsePriceField(fields[2], lineNo),
              ParsePriceField(fields[3], lineNo),
              ParsePriceField(fields[4], lineNo)};

      GetContext().SetCurrentTime(time, true);
      // @todo fixme m_security->SetLevel1(time, values, Milestones());
      const auto &lastPrice = *bar.closePrice;
      const auto halfOfSpread = lastPrice * 0.005;
      m_security->AddLevel1Tick(
          time,
          Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(lastPrice +
                                                         halfOfSpread),
          Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(lastPrice -
                                                         halfOfSpread),
          Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(*bar.closePrice),
          Milestones());
      GetContext().SyncDispatching();
    }

    GetLog().Info("Read %1% lines from %2%.", lineNo, m_settings.filePath);
  }

 private:
  template <typename Fields>
  gr::date ParseDateField(const Fields &fields, size_t lineNo) const {
    AssertGt(fields.size(), m_settings.dateFieldIndex);
    /* @todo fixme AssertGt(m_settings.fields.size(),
m_settings.dateFieldIndex);
AssertEq(numberOfLevel1TickTypes,
         m_settings.fields[m_settings.dateFieldIndex]); */

    const auto &field = fields[m_settings.dateFieldIndex];
    if (m_settings.dateFieldIndex != m_settings.timeFieldIndex) {
      switch (field.size()) {
        case 8:
          return gr::from_undelimited_string(field);
        case 10:
          return gr::from_string(field);
        default:
          GetLog().Error("Unknown date field format at line %1%.", lineNo);
          throw Exception("Wrong file format");
      }
    } else {
      return gr::from_undelimited_string(field.substr(0, 8));
    }
  }

  template <typename Fields>
  pt::time_duration ParseTimeField(const Fields &fields, size_t lineNo) const {
    AssertGt(fields.size(), m_settings.timeFieldIndex);
    // @todo fixme AssertGt(m_settings.fields.size(),
    // m_settings.timeFieldIndex);
    /* @todo fixme AssertEq(numberOfLevel1TickTypes,
             m_settings.fields[m_settings.timeFieldIndex]); */

    const auto &field = fields[m_settings.timeFieldIndex];
    if (m_settings.dateFieldIndex != m_settings.timeFieldIndex) {
      switch (field.size()) {
        case 6:
          return pt::duration_from_string(field.substr(0, 2) + ":" +
                                          field.substr(2, 2) + ":" +
                                          field.substr(4, 2));
        case 5:
        case 8:
          return pt::duration_from_string(field);
        default:
          GetLog().Error("Unknown time field format at line %1%.", lineNo);
          throw Exception("Wrong file format");
      }
    }
    return pt::duration_from_string(field.substr(9, 2) + ":" +
                                    field.substr(11, 2) + ":" +
                                    field.substr(13, 2));
  }

  template <typename Field>
  Price ParsePriceField(const Field &field, size_t lineNo) const {
    try {
      return boost::lexical_cast<double>(field);
    } catch (const boost::bad_lexical_cast &) {
      GetLog().Error("Wrong price field format \"%1%\" at line %2%.", field,
                     lineNo);
      throw Exception("Wrong file format");
    }
  }

  const Settings m_settings;
  boost::shared_ptr<Test::Security> m_security;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_TEST_API boost::shared_ptr<trdk::MarketDataSource>
CreateCsvTickMarketDataSource(Context &context,
                              std::string instanceName,
                              std::string title,
                              const ptr::ptree &configuration) {
  return boost::make_shared<CsvTickMarketDataSource>(
      context, std::move(instanceName), std::move(title), configuration);
}

////////////////////////////////////////////////////////////////////////////////
