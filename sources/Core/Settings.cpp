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
#include <utility>

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;
namespace lt = boost::local_time;

using namespace trdk;
using namespace Lib;

namespace {
bool IsCurrencySupported(const std::string &symbol) {
  const auto delimeter = symbol.find('_');
  if (delimeter == std::string::npos) {
    return true;
  }
  try {
    ConvertCurrencyFromIso(symbol.substr(delimeter + 1));
  } catch (const Exception &) {
    return false;
  }
  return true;
}
}  // namespace

class Settings::Implementation {
 public:
  ptr::ptree m_config;
  SecurityType m_defaultSecurityType;
  Currency m_defaultCurrency;
  bool m_isReplayMode;
  bool m_isMarketDataLogEnabled;
  pt::ptime m_startTime;
  fs::path m_logsDir;
  lt::time_zone_ptr m_timeZone;
  gr::date_duration m_periodBeforeExpiryDayToSwitchContract;
  boost::unordered_map<std::string, std::string> m_symbolAliases;
  boost::unordered_set<std::string> m_defaultSymbols;

  Implementation()
      : m_defaultSecurityType(numberOfSecurityTypes),
        m_defaultCurrency(numberOfCurrencies),
        m_isReplayMode(false),
        m_isMarketDataLogEnabled(false),
        m_timeZone(boost::make_shared<lt::posix_time_zone>("GMT")) {}

  Implementation(const fs::path &confFile,
                 fs::path &&logsDir,
                 const pt::ptime &universalStartTime)
      : m_defaultSecurityType(numberOfSecurityTypes),
        m_defaultCurrency(numberOfCurrencies),
        m_isReplayMode(false),
        m_isMarketDataLogEnabled(false),
        m_logsDir(std::move(logsDir)) {
    LoadConfig(confFile);

    m_startTime =
        lt::local_date_time(universalStartTime, m_timeZone).local_time();

    if (m_isReplayMode) {
      m_logsDir /= "Replay_" + ConvertToFileName(m_startTime);
    } else {
      m_logsDir /= ConvertToFileName(m_startTime);
    }
  }

  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = delete;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() = default;

  void Log(Context::Log &log) const {
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

 private:
  void LoadConfig(const fs::path &filePath) {
    std::ifstream file(filePath.string().c_str());
    if (!file) {
      boost::format error(R"(Failed to open configuration file %1%)");
      error % filePath;
      throw Exception(error.str().c_str());
    }

    try {
      ptr::read_json(file, m_config);
      const auto &commonConf = m_config.get_child("general");
      const auto &defaultsConf = m_config.get_child("defaults");

      m_isReplayMode = commonConf.get<bool>("isReplayMode");
      m_isMarketDataLogEnabled =
          commonConf.get<bool>("marketDataLog.isEnabled");

      {
        std::string timeZone;
        const auto &timeZoneConfig =
            commonConf.get_optional<std::string>("timeZone");
        if (timeZoneConfig) {
          timeZone = *timeZoneConfig;
        } else {
          const auto &diff = pt::second_clock::local_time() -
                             pt::second_clock::universal_time();
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
        const auto &currency = defaultsConf.get<std::string>("currency");
        try {
          m_defaultCurrency = ConvertCurrencyFromIso(currency);
        } catch (const Exception &ex) {
          boost::format error(
              R"(Failed to parse default currency ISO 4217 code "%1%": "%2%")");
          error % currency % ex.what();
          throw Exception(error.str().c_str());
        }
      }
      {
        const auto &conf = defaultsConf.get_child_optional("symbols");
        if (conf) {
          for (const auto &node : *conf) {
            auto symbol = node.second.get_value<std::string>();
            if (!IsCurrencySupported(symbol)) {
              continue;
            }
            m_defaultSymbols.emplace(std::move(symbol));
          }
        }
      }
      {
        const auto &securityType =
            defaultsConf.get<std::string>("securityType");
        try {
          m_defaultSecurityType = ConvertSecurityTypeFromString(securityType);
        } catch (const Exception &ex) {
          boost::format error(
              R"(Failed to parse default security type "%1%": "%2%")");
          error % securityType % ex.what();
          throw Exception(error.str().c_str());
        }
      }

      {
        const auto &aliases = commonConf.get_child_optional("symbolAliases");
        if (aliases) {
          for (const auto &node : *aliases) {
            const auto &symbol = node.first;
            const auto &alias = node.second.get_value<std::string>();
            if (symbol.empty() || alias.empty()) {
              throw Exception(
                  "One or more symbol aliases have an invalid format");
            }
            m_symbolAliases.emplace(symbol, alias);
          }
        }
      }

    } catch (const ptr::ptree_error &ex) {
#ifndef DEV_VER
      {
        UseUnused(ex);
        boost::format error("The configuration file %1% has a wrong format");
        error % ex.what();  // 1
        throw Exception(error.str().c_str());
      }
#else
      {
        boost::format error(
            R"(The configuration file %1% has a wrong format: "%2%")");
        error % filePath  // 1
            % ex.what();  // 2
        throw Exception(error.str().c_str());
      }
#endif
    }
  }
};

Settings::Settings() : m_pimpl(boost::make_unique<Implementation>()) {}
Settings::Settings(const fs::path &confFile,
                   fs::path logsDir,
                   const pt::ptime &universalStartTime)
    : m_pimpl(boost::make_unique<Implementation>(
          confFile, std::move(logsDir), universalStartTime)) {}
Settings::Settings(Settings &&) = default;
Settings &Settings::operator=(Settings &&) = default;
Settings::~Settings() = default;

void Settings::Log(Context::Log &log) const { m_pimpl->Log(log); }

const ptr::ptree &Settings::GetConfig() const { return m_pimpl->m_config; }

const pt::ptime &Settings::GetStartTime() const { return m_pimpl->m_startTime; }

bool Settings::IsReplayMode() const noexcept { return m_pimpl->m_isReplayMode; }

bool Settings::IsMarketDataLogEnabled() const {
  return m_pimpl->m_isMarketDataLogEnabled;
}

const fs::path &Settings::GetLogsDir() const { return m_pimpl->m_logsDir; }

const Currency &Settings::GetDefaultCurrency() const {
  return m_pimpl->m_defaultCurrency;
}

const SecurityType &Settings::GetDefaultSecurityType() const {
  return m_pimpl->m_defaultSecurityType;
}

const lt::time_zone_ptr &Settings::GetTimeZone() const {
  return m_pimpl->m_timeZone;
}

const gr::date_duration &Settings::GetPeriodBeforeExpiryDayToSwitchContract()
    const {
  return m_pimpl->m_periodBeforeExpiryDayToSwitchContract;
}

const std::string &Settings::ResolveSymbolAlias(
    const std::string &symbol) const {
  const auto &it = m_pimpl->m_symbolAliases.find(symbol);
  return it == m_pimpl->m_symbolAliases.cend() ? symbol : it->second;
}

void Settings::ReplaceSymbolWithAlias(std::string &symbolToReplace) const {
  const auto &it = m_pimpl->m_symbolAliases.find(symbolToReplace);
  if (it == m_pimpl->m_symbolAliases.cend()) {
    return;
  }
  symbolToReplace = it->second;
}

const boost::unordered_set<std::string> &Settings::GetDefaultSymbols() const {
  return m_pimpl->m_defaultSymbols;
}
