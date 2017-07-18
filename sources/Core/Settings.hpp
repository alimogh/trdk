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
  explicit Settings(const trdk::Lib::Ini &,
                    const boost::posix_time::ptime &universalStartTime);

 public:
  void Log(trdk::Context::Log &) const;

 public:
  const boost::posix_time::ptime &GetStartTime() const { return m_startTime; }

  bool IsReplayMode() const noexcept { return m_isReplayMode; }

  bool IsMarketDataLogEnabled() const { return m_isMarketDataLogEnabled; }

  const boost::filesystem::path &GetLogsRootDir() const {
    return m_logsRootDir;
  }
  const boost::filesystem::path &GetLogsInstanceDir() const {
    return m_logsInstanceDir;
  }

  //! Default security Currency.
  /** Path: Defaults::currency
    * Ex.: "currency = USD"
    */
  const trdk::Lib::Currency &GetDefaultCurrency() const {
    return m_defaultCurrency;
  }

  //! Default security Security Type.
  /** Path: Defaults::currency.
    * Values: STK, FUT, FOP, FOR, FORFOP
    * Ex.: "security_type = FOP"
    */
  const trdk::Lib::SecurityType &GetDefaultSecurityType() const {
    return m_defaultSecurityType;
  }

  const boost::local_time::time_zone_ptr &GetTimeZone() const {
    return m_timeZone;
  }

 private:
  const trdk::Lib::SecurityType m_defaultSecurityType;
  const trdk::Lib::Currency m_defaultCurrency;
  const bool m_isReplayMode;
  const bool m_isMarketDataLogEnabled;
  const boost::posix_time::ptime m_startTime;
  const boost::filesystem::path m_logsRootDir;
  const boost::filesystem::path m_logsInstanceDir;
  const boost::local_time::time_zone_ptr m_timeZone;
};
}
