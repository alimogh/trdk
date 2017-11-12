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

class FloodControl : private boost::noncopyable {
 public:
  virtual ~FloodControl() = default;

 public:
  virtual void Check(bool isPriority) = 0;
};

class DisabledFloodControl : public FloodControl {
 public:
  virtual ~DisabledFloodControl() override = default;

 public:
  virtual void Check(bool) {}
};

class MinTimeBetweenRequestsFloodControl : public FloodControl {
 private:
  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

 public:
  explicit MinTimeBetweenRequestsFloodControl(
      const boost::posix_time::time_duration &minTimeBetweenRequests);
  virtual ~MinTimeBetweenRequestsFloodControl() override = default;

 public:
  virtual void Check(bool isPriority) override;

 private:
  boost::posix_time::ptime CalcNextRequestTime() const;

 private:
  const boost::posix_time::time_duration m_minTimeBetweenRequests;
  Mutex m_mutex;
  boost::posix_time::ptime m_nextRequestTime;
};
}
}
}