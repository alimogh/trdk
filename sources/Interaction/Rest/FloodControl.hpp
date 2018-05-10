/*******************************************************************************
 *   Created: 2017/11/12 13:28:44
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
namespace Rest {

////////////////////////////////////////////////////////////////////////////////

class TRDK_INTERACTION_REST_API FloodControl : boost::noncopyable {
 public:
  virtual ~FloodControl() = default;

  virtual void Check(bool isPriority, ModuleEventsLog &) = 0;
  virtual void OnRateLimitExceeded();
};

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_REST_API std::unique_ptr<FloodControl>
CreateDisabledFloodControl();

std::unique_ptr<FloodControl> CreateFloodControlWithMinTimeBetweenRequests(
    const boost::posix_time::time_duration &minTimeBetweenRequests,
    const boost::posix_time::time_duration &criticalWaitTimeToReport);

std::unique_ptr<FloodControl> CreateFloodControlWithMaxRequestsPerPeriod(
    size_t maxNumberOfRequest,
    const boost::posix_time::time_duration &period,
    const boost::posix_time::time_duration &criticalWaitTimeToReport);

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk