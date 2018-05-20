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
      throw Error("Failed to open INI-file");
    }
  }
  ~IniFile() override = default;

 protected:
  std::istream &GetSource() const override { return m_file; }

 private:
  static fs::path FindIniFile(const fs::path &source) {
    if (boost::iends_with(source.string(), ".ini")) {
      return source;
    }
    if (fs::exists(source)) {
      return source;
    }
    const fs::path pathWhithExt = source.string() + ".ini";
    if (!fs::exists(pathWhithExt)) {
      return source;
    }
    return pathWhithExt;
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
                   const pt::ptime &universalStartTime)
    : m_ini(boost::make_unique<IniFile>(confFile)),
      m_defaultSecurityType(numberOfSecurityTypes),
      m_defaultCurrency(numberOfCurrencies),
      m_isReplayMode(false),
      m_isMarketDataLogEnabled(false) {
  const IniSectionRef commonConf(*m_ini, "General");
  const IniSectionRef defaultsConf(*m_ini, "Defaults");

  const_cast<bool &>(m_isReplayMode) = commonConf.ReadBoolKey("is_replay_mode");

  const_cast<bool &>(m_isMarketDataLogEnabled) =
      commonConf.ReadBoolKey("market_data_log");

  const_cast<lt::time_zone_ptr &>(m_timeZone) =
      boost::make_shared<lt::posix_time_zone>(commonConf.ReadKey("timezone"));

  {
    const auto *const currencyKey = "currency";
    auto currency = defaultsConf.ReadKey(currencyKey);
    if (!currency.empty()) {
      try {
        const_cast<Currency &>(m_defaultCurrency) =
            ConvertCurrencyFromIso(currency);
      } catch (const Exception &ex) {
        boost::format error(
            "Failed to parse default currency ISO 4217 code"
            " \"%1%\": \"%2%\"");
        error % currency % ex.what();
        throw Exception(error.str().c_str());
      }
    }
  }
  {
    const char *const securityTypeKey = "security_type";
    std::string securityType = defaultsConf.ReadKey(securityTypeKey);
    if (!securityType.empty()) {
      try {
        const_cast<SecurityType &>(m_defaultSecurityType) =
            ConvertSecurityTypeFromString(securityType);
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
      const_cast<gr::date_duration &>(m_periodBeforeExpiryDayToSwitchContract) =
          gr::days(commonConf.ReadTypedKey<long>(key));
    }
  }

  {
    const auto *const key = "symbol_aliases";
    if (commonConf.IsKeyExist(key)) {
      for (const auto &item : commonConf.ReadList(key, ",", false)) {
        const auto &delimter = item.find("=");
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
        const_cast<boost::unordered_map<std::string, std::string> &>(
            m_symbolAliases)
            .emplace(std::move(symbol), std::move(alias));
      }
    }
  }

  const_cast<pt::ptime &>(m_startTime) =
      lt::local_date_time(universalStartTime, m_timeZone).local_time();

  const_cast<fs::path &>(m_logsRootDir) =
      commonConf.ReadFileSystemPath("logs_dir");
#ifdef _DEBUG
  const_cast<fs::path &>(m_logsRootDir) /= "DEBUG";
#endif
  const_cast<fs::path &>(m_logsInstanceDir) = m_logsRootDir;
  if (m_isReplayMode) {
    const_cast<fs::path &>(m_logsInstanceDir) /=
        "Replay_" + ConvertToFileName(m_startTime);
  } else {
    const_cast<fs::path &>(m_logsInstanceDir) /= ConvertToFileName(m_startTime);
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
