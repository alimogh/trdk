/*******************************************************************************
 *   Created: 2017/12/12 10:20:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;

FloodControl &LivecoinRequest::GetFloodControl() {
  static class FloodControl : public Rest::FloodControl {
   private:
    typedef boost::mutex Mutex;
    typedef Mutex::scoped_lock Lock;

   public:
    FloodControl() : m_events(60) {}
    virtual ~FloodControl() override = default;

   public:
    virtual void Check(bool isPriority) override {
      Lock lock(m_mutex);

      static const auto period = pt::seconds(1);
      auto now = pt::microsec_clock::universal_time();
      FlushEvents(now - period);

      while (m_events.full()) {
        AssertEq(m_events.capacity(), m_events.size());
        const auto timeToWait = m_events.front() + period - now;
        AssertLt(pt::microseconds(0), timeToWait);
        Assert(!timeToWait.is_negative());
        if (!isPriority) {
          lock.unlock();
        }
        boost::this_thread::sleep(timeToWait);
        if (!isPriority) {
          lock.lock();
        }
        now = pt::microsec_clock::universal_time();
        FlushEvents(now - period);
      }

      m_events.push_back(now);
    }

   private:
    void FlushEvents(const pt::ptime &startTime) {
      while (!m_events.empty() && m_events.front() <= startTime) {
        m_events.pop_front();
      }
    }

   private:
    Mutex m_mutex;
    boost::circular_buffer<pt::ptime> m_events;
  } result;

  return result;
}