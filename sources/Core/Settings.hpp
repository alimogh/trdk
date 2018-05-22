/**************************************************************************
 *   Created: 2012/07/13 20:03:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Context.hpp"

namespace trdk {

class TRDK_CORE_API Settings {
 public:
  Settings();
  explicit Settings(const boost::filesystem::path&,
                    const boost::posix_time::ptime& universalStartTime);
  Settings(Settings&&) = default;
  Settings(const Settings&) = delete;
  Settings& operator=(Settings&&) = default;
  Settings& operator=(const Settings&) = delete;
  ~Settings() = default;

  const Lib::Ini& GetConfig() const { return *m_ini; }

  void Log(Context::Log&) const;

  const boost::posix_time::ptime& GetStartTime() const { return m_startTime; }

  bool IsReplayMode() const noexcept { return m_isReplayMode; }

  bool IsMarketDataLogEnabled() const { return m_isMarketDataLogEnabled; }

  const boost::filesystem::path& GetLogsRootDir() const {
    return m_logsRootDir;
  }

  const boost::filesystem::path& GetLogsInstanceDir() const {
    return m_logsInstanceDir;
  }

  //! Default security Currency.
  /**
   * Path: Defaults::currency
   * Ex.: "currency = USD"
   */
  const Lib::Currency& GetDefaultCurrency() const { return m_defaultCurrency; }

  //! Default security Security Type.
  /**
   * Path: Defaults::currency.
   * Values: STK, FUT, FOP, FOR, FORFOP
   * Ex.: "security_type = FOP"
   */
  const Lib::SecurityType& GetDefaultSecurityType() const {
    return m_defaultSecurityType;
  }

  const boost::local_time::time_zone_ptr& GetTimeZone() const {
    return m_timeZone;
  }

  const boost::gregorian::date_duration&
  GetPeriodBeforeExpiryDayToSwitchContract() const {
    return m_periodBeforeExpiryDayToSwitchContract;
  }

  const std::string& ResolveSymbolAlias(const std::string&) const;
  void ReplaceSymbolWithAlias(std::string&) const;

 private:
  std::unique_ptr<Lib::Ini> m_ini;
  Lib::SecurityType m_defaultSecurityType;
  Lib::Currency m_defaultCurrency;
  bool m_isReplayMode;
  bool m_isMarketDataLogEnabled;
  boost::posix_time::ptime m_startTime;
  boost::filesystem::path m_logsRootDir;
  boost::filesystem::path m_logsInstanceDir;
  boost::local_time::time_zone_ptr m_timeZone;
  boost::gregorian::date_duration m_periodBeforeExpiryDayToSwitchContract;
  boost::unordered_map<std::string, std::string> m_symbolAliases;
};

}  // namespace trdk
