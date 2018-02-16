/**************************************************************************
 *   Created: 2014/08/30 10:44:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/atomic.hpp>

namespace trdk {
namespace Lib {
namespace Concurrency {

class SpinScopedLock;

////////////////////////////////////////////////////////////////////////////////

class SpinMutex {
 public:
  typedef SpinScopedLock ScopedLock;

 public:
  SpinMutex() {}
  SpinMutex(SpinMutex &&) = default;

 private:
  SpinMutex(const SpinMutex &);
  const SpinMutex &operator=(const SpinMutex &);

 public:
  void Lock() {
    while (m_state.test_and_set(boost::memory_order_acquire))
      ;
  }

  void Unlock() { m_state.clear(); }

 private:
  boost::atomic_flag m_state;
};

class SpinScopedLock : private boost::noncopyable {
 public:
  explicit SpinScopedLock(SpinMutex &mutex) : m_mutex(mutex) { Lock(); }

  ~SpinScopedLock() { Unlock(); }

 public:
  SpinMutex &GetMutex() { return m_mutex; }
  SpinMutex *mutex() { return &GetMutex(); }
  const SpinMutex &GetMutex() const {
    return const_cast<SpinScopedLock *>(this)->GetMutex();
  }
  const SpinMutex *mutex() const { return &GetMutex(); }

  void Lock() { m_mutex.Lock(); }
  void lock() { Lock(); }

  void Unlock() { m_mutex.Unlock(); }
  void unlock() { Unlock(); }

 private:
  SpinMutex &m_mutex;
};

////////////////////////////////////////////////////////////////////////////////

class SpinCondition : private boost::noncopyable {
 public:
  SpinCondition() : m_waitersCount(0) {}

  ~SpinCondition() { AssertEq(0, m_waitersCount); }

 public:
  void NotifyOne() { m_state.clear(); }

  void notify_one() { NotifyOne(); }

  void NotifyAll() {
    while (m_waitersCount != 0) {
      NotifyOne();
    }
  }

  void notify_all() { NotifyAll(); }

  void Wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const {
    if (!m_state.test_and_set(boost::memory_order_acquire)) {
      return;
    }
    ++m_waitersCount;
    lock.Unlock();
    while (m_state.test_and_set(boost::memory_order_acquire))
      ;
    AssertLt(0, m_waitersCount);
    --m_waitersCount;
    lock.Lock();
  }

  void wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const { Wait(lock); }

 private:
  mutable boost::atomic_size_t m_waitersCount;
  mutable boost::atomic_flag m_state;
};

////////////////////////////////////////////////////////////////////////////////

class LazySpinCondition : private boost::noncopyable {
 public:
  LazySpinCondition() : m_waitersCount(0) {}

  ~LazySpinCondition() { AssertEq(0, m_waitersCount); }

 public:
  void NotifyOne() { m_state.clear(); }

  void notify_one() { NotifyOne(); }

  void NotifyAll() {
    while (m_waitersCount != 0) {
      NotifyOne();
    }
  }

  void notify_all() { NotifyAll(); }

  void Wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const {
    if (!m_state.test_and_set(boost::memory_order_acquire)) {
      return;
    }
    ++m_waitersCount;
    lock.Unlock();
    while (m_state.test_and_set(boost::memory_order_acquire)) {
      boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
    AssertLt(0, m_waitersCount);
    --m_waitersCount;
    lock.Lock();
  }

  void wait(trdk::Lib::Concurrency::SpinScopedLock &lock) const { Wait(lock); }

 private:
  mutable boost::atomic_size_t m_waitersCount;
  mutable boost::atomic_flag m_state;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Concurrency
}  // namespace Lib
}  // namespace trdk
