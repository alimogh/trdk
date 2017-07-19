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
#include "Security.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace lt = boost::local_time;

using namespace trdk;
using namespace trdk::Lib;

Settings::Settings()
    : m_defaultSecurityType(numberOfSecurityTypes),
      m_defaultCurrency(numberOfCurrencies),
      m_isReplayMode(false),
      m_isMarketDataLogEnabled(false) {}

Settings::Settings(const Ini &conf, const pt::ptime &universalStartTime)
    : m_defaultSecurityType(numberOfSecurityTypes),
      m_defaultCurrency(numberOfCurrencies),
      m_isReplayMode(false),
      m_isMarketDataLogEnabled(false) {
  const IniSectionRef commonConf(conf, "General");
  const IniSectionRef defaultsConf(conf, "Defaults");

  const_cast<bool &>(m_isReplayMode) = commonConf.ReadBoolKey("is_replay_mode");

  const_cast<bool &>(m_isMarketDataLogEnabled) =
      commonConf.ReadBoolKey("market_data_log");

  const_cast<lt::time_zone_ptr &>(m_timeZone) =
      boost::make_shared<lt::posix_time_zone>(commonConf.ReadKey("timezone"));

  {
    const char *const currencyKey = "currency";
    std::string currency = defaultsConf.ReadKey(currencyKey);
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
  log.Info(
      "Timezone: %1%. Default currenct: %2%. Default security type: %3%."
      " Market data log: %4%.",
      m_timeZone->to_posix_string(), m_defaultCurrency, m_defaultSecurityType,
      m_isMarketDataLogEnabled ? "enabled" : "disabled");
}
