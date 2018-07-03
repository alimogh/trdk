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
  explicit Settings(const boost::filesystem::path& confFile,
                    boost::filesystem::path logsDir,
                    const boost::posix_time::ptime& universalStartTime);
  Settings(Settings&&);
  Settings(const Settings&) = delete;
  Settings& operator=(Settings&&);
  Settings& operator=(const Settings&) = delete;
  ~Settings();

  const boost::property_tree::ptree& GetConfig() const;

  void Log(Context::Log&) const;

  const boost::posix_time::ptime& GetStartTime() const;

  bool IsReplayMode() const noexcept;

  bool IsMarketDataLogEnabled() const;

  const boost::filesystem::path& GetLogsDir() const;

  //! Default security Currency.
  /**
   * Path: Defaults::currency
   * Ex.: "currency = USD"
   */
  const Lib::Currency& GetDefaultCurrency() const;

  //! Default security Security Type.
  /**
   * Path: Defaults::currency.
   * Values: STK, Futures, FOP, For, FORFOP
   * Ex.: "security_type = FOP"
   */
  const Lib::SecurityType& GetDefaultSecurityType() const;

  const boost::local_time::time_zone_ptr& GetTimeZone() const;

  const boost::gregorian::date_duration&
  GetPeriodBeforeExpiryDayToSwitchContract() const;

  const std::string& ResolveSymbolAlias(const std::string&) const;
  void ReplaceSymbolWithAlias(std::string&) const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace trdk
