/*******************************************************************************
 *   Created: 2017/11/12 13:43:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FloodControl.hpp"

using namespace trdk;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;

MinTimeBetweenRequestsFloodControl::MinTimeBetweenRequestsFloodControl(
    const pt::time_duration &minTimeBetweenRequests)
    : m_minTimeBetweenRequests(minTimeBetweenRequests),
      m_nextRequestTime(pt::microsec_clock::universal_time() +
                        m_minTimeBetweenRequests) {}

void MinTimeBetweenRequestsFloodControl::Check(bool isPriority) {
  Lock lock(m_mutex);
  auto now = pt::microsec_clock::universal_time();
  auto timeToWait = m_nextRequestTime - now;
  while (!timeToWait.is_negative()) {
    if (!isPriority) {
      lock.unlock();
    }
    boost::this_thread::sleep(timeToWait);
    if (!isPriority) {
      lock.lock();
    }
    now = pt::microsec_clock::universal_time();
    timeToWait = m_nextRequestTime - now;
  }
  m_nextRequestTime = now + m_minTimeBetweenRequests;
}
