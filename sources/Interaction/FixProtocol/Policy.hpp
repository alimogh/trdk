/*******************************************************************************
 *   Created: 2017/09/21 23:46:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {
//! FIX protocol policy.
/** Each policy is a module in cpp-file. At FixProtocol start each policy adds
  * own name (for ex. "FIX4.4" or "cTrade") into the static map<name, fabric>.
  * Settings object creates policy at start. Unused policies will be removed
  * from custom branch.
  */
class Policy : private boost::noncopyable {
 public:
  explicit Policy(const trdk::Settings &);

 public:
  boost::posix_time::ptime ConvertFromRemoteTime(
      const boost::posix_time::ptime &source) const {
    return source + m_utcDiff;
  }
  boost::posix_time::ptime GetCurrentTime() const {
    return boost::posix_time::second_clock::universal_time();
  }

  bool IsUnknownOrderIdError(const std::string &errorText);
  bool IsBadTradingVolumeError(const std::string &errorText);

 private:
  const boost::posix_time::time_duration m_utcDiff;
};
}
}
}