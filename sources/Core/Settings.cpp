/**************************************************************************
 *   Created: 2012/07/13 21:13:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Settings.hpp"

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;
namespace lt = boost::local_time;

using namespace trdk;
using namespace Lib;

namespace {

class IniFile : public Ini {
 public:
  explicit IniFile(const fs::path &path)
      : m_file(FindIniFile(path).string().c_str()) {
    if (!m_file) {
      throw Error("Failed to open configuration file");
    }
  }
  ~IniFile() override = default;

 protected:
  std::istream &GetSource() const override { return m_file; }

 private:
  static fs::path FindIniFile(const fs::path &source) {
    if (fs::exists(source)) {
      return source;
    }
    try {
      fs::create_directories(source.branch_path());
    } catch (const fs::filesystem_error &ex) {
      boost::format error("Failed to create default configuration file");
      error % ex.what();
      throw Error(error.str().c_str());
    }
    {
      std::ofstream file(source.string().c_str());
      if (!file) {
        throw Error("Failed to create default configuration file");
      }

      file << "[General]" << std::endl;
      file << "\tis_replay_mode = no" << std::endl;
      file << std::endl;
      file << "\ttrading_log = yes" << std::endl;
      file << "\tmarket_data_log = no" << std::endl;
      file << std::endl;
      file << "\t; When switch contract to the next. Zero - at the first"
           << std::endl;
      file << "\t; minutes of the expiration day. One - one day before"
           << std::endl;
      file << "\t; of the expiration day." << std::endl;
      file << "\tnumber_of_days_before_expiry_day_to_switch_contract = 0"
           << std::endl;

      file << std::endl;
      file << "[Defaults]" << std::endl;
      file << "\tcurrency = BTC" << std::endl;
      file << "\tsecurity_type = CRYPTO" << std::endl;
      file << "\tsymbol_list = BTC_EUR, BTC_USD, ETH_BTC, ETH_EUR, ETH_USD, "
              "DOGE_BTC, LTC_BTC, DASH_BTC, BCH_BTC"
           << std::endl;

      file << std::endl;
      file << "[RiskControl]" << std::endl;
      file << "\tis_enabled = false" << std::endl;
    }
    return source;
  }

  mutable std::ifstream m_file;
};

}  // namespace

Settings::Settings()
    : m_defaultSecurityType(numberOfSecurityTypes),
      m_defaultCurrency(numberOfCurrencies),
      m_isReplayMode(false),
      m_isMarketDataLogEnabled(false),
      m_timeZone(boost::make_shared<lt::posix_time_zone>("GMT")) {}

Settings::Settings(const fs::path &confFile,
                   const fs::path &logsDir,
                   const pt::ptime &universalStartTime)
    : m_ini(boost::make_unique<IniFile>(confFile)),
      m_defaultSecurityType(numberOfSecurityTypes),
      m_defaultCurrency(numberOfCurrencies),
      m_isReplayMode(false),
      m_isMarketDataLogEnabled(false),
      m_logsDir(logsDir) {
  const IniSectionRef commonConf(*m_ini, "General");
  const IniSectionRef defaultsConf(*m_ini, "Defaults");

  m_isReplayMode = commonConf.ReadBoolKey("is_replay_mode");

  m_isMarketDataLogEnabled = commonConf.ReadBoolKey("market_data_log");

  {
    std::string timeZone;
    if (commonConf.IsKeyExist("timezone")) {
      timeZone = commonConf.ReadKey("timezone");
    } else {
      const auto &diff =
          pt::second_clock::local_time() - pt::second_clock::universal_time();
      timeZone = (boost::format("GMT%1%%2%:%3%") %
                  (diff.is_negative() ? '-' : '+')  // 1
                  % boost::io::group(std::setw(2), std::setfill('0'),
                                     diff.hours())  // 2
                  % boost::io::group(std::setw(2), std::setfill('0'),
                                     (diff.minutes() / 15) * 15))  // 3
                     .str();
    }
    m_timeZone = boost::make_shared<lt::posix_time_zone>(timeZone);
  }

  {
    const auto *const currencyKey = "currency";
    auto currency = defaultsConf.ReadKey(currencyKey);
    if (!currency.empty()) {
      try {
        m_defaultCurrency = ConvertCurrencyFromIso(currency);
      } catch (const Exception &ex) {
        boost::format error(
            R"(Failed to parse default currency ISO 4217 code "%1%": "%2%")");
        error % currency % ex.what();
        throw Exception(error.str().c_str());
      }
    }
  }
  {
    const auto securityTypeKey = "security_type";
    std::string securityType = defaultsConf.ReadKey(securityTypeKey);
    if (!securityType.empty()) {
      try {
        m_defaultSecurityType = ConvertSecurityTypeFromString(securityType);
      } catch (const Exception &ex) {
        boost::format error(
            "Failed to parse default security type"
            " \"%1%\": \"%2%\"");
        error % securityType % ex.what();
        throw Exception(error.str().c_str());
      }
    }
  }

  {
    const auto *const key =
        "number_of_days_before_expiry_day_to_switch_contract";
    if (commonConf.IsKeyExist(key)) {
      m_periodBeforeExpiryDayToSwitchContract =
          gr::days(commonConf.ReadTypedKey<long>(key));
    }
  }

  {
    const auto *const key = "symbol_aliases";
    if (commonConf.IsKeyExist(key)) {
      for (const auto &item : commonConf.ReadList(key, ",", false)) {
        const auto &delimter = item.find('=');
        std::string symbol;
        std::string alias;
        if (delimter != std::string::npos) {
          symbol = boost::trim_copy(item.substr(0, delimter));
          alias = boost::trim_copy(item.substr(delimter + 1));
        }
        if (symbol.empty() || alias.empty()) {
          boost::format error("Symbol alias \"%1%\" has invalid format");
          error % alias;
          throw Exception(error.str().c_str());
        }
        m_symbolAliases.emplace(std::move(symbol), std::move(alias));
      }
    }
  }

  m_startTime =
      lt::local_date_time(universalStartTime, m_timeZone).local_time();

  if (m_isReplayMode) {
    m_logsDir /= "Replay_" + ConvertToFileName(m_startTime);
  } else {
    m_logsDir /= ConvertToFileName(m_startTime);
  }
}

void Settings::Log(Context::Log &log) const {
  if (m_isReplayMode) {
    log.Info("======================= REPLAY MODE =======================");
  }
  log.Debug(
      "Timezone: %1%. Default currency: %2%. Default security type: %3%."
      " Market data log: %4%. Contract switching before: %5%.",
      m_timeZone->to_posix_string(),                      // 1
      m_defaultCurrency,                                  // 2
      m_defaultSecurityType,                              // 3
      m_isMarketDataLogEnabled ? "enabled" : "disabled",  // 4
      m_periodBeforeExpiryDayToSwitchContract);           // 5
}

const std::string &Settings::ResolveSymbolAlias(
    const std::string &symbol) const {
  const auto &it = m_symbolAliases.find(symbol);
  return it == m_symbolAliases.cend() ? symbol : it->second;
}

void Settings::ReplaceSymbolWithAlias(std::string &symbolToReplace) const {
  const auto &it = m_symbolAliases.find(symbolToReplace);
  if (it == m_symbolAliases.cend()) {
    return;
  }
  symbolToReplace = it->second;
}
