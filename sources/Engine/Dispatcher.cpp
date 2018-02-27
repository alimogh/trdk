/**************************************************************************
 *   Created: 2012/11/22 11:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Core/Observer.hpp"
#include "Core/Strategy.hpp"
#include "Context.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;
using namespace trdk::Engine::Details;

////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////

struct NoMeasurementPolicy {
  static TimeMeasurement::Milestones StartDispatchingTimeMeasurement(
      const trdk::Context &) {
    return TimeMeasurement::Milestones();
  }
};

struct DispatchingTimeMeasurementPolicy {
  static TimeMeasurement::Milestones StartDispatchingTimeMeasurement(
      const trdk::Context &context) {
    return context.StartDispatchingTimeMeasurement();
  }
};

////////////////////////////////////////////////////////////////////////////////

struct StopVisitor : public boost::static_visitor<> {
  template <typename T>
  void operator()(T *obj) const {
    obj->Stop();
  }
};

struct ActivateVisitor : public boost::static_visitor<> {
  template <typename T>
  void operator()(T *obj) const {
    obj->Activate();
  }
};

struct SuspendVisitor : public boost::static_visitor<> {
  template <typename T>
  void operator()(T *obj) const {
    obj->Suspend();
  }
};

struct IsActiveVisitor : public boost::static_visitor<bool> {
  template <typename T>
  bool operator()(const T *obj) const {
    return obj->IsActive();
  }
};

struct SyncVisitor : public boost::static_visitor<> {
  template <typename T>
  void operator()(T *obj) const {
    obj->Sync();
  }
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher(Engine::Context &context)
    : m_context(context),
      m_level1Updates("Level 1 updates", m_context),
      m_level1Ticks("Level 1 ticks", m_context),
      m_newTrades("Trades", m_context),
      m_positionsUpdates("Positions", m_context),
      m_brokerPositionsUpdates("Broker positions", m_context),
      m_newBars("System bars", m_context),
      m_bookUpdateTicks("Book update ticks", m_context) {
  m_queues.emplace_back(&m_level1Updates);
  m_queues.emplace_back(&m_level1Ticks);
  m_queues.emplace_back(&m_newTrades);
  m_queues.emplace_back(&m_positionsUpdates);
  m_queues.emplace_back(&m_brokerPositionsUpdates);
  m_queues.emplace_back(&m_newBars);
  m_queues.emplace_back(&m_bookUpdateTicks);
  m_queues.shrink_to_fit();

#if TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS || TRDK_USAGE_STOP_TIMESTAMP_MS
  {
    if (context.GetCurrentTime() >=
        ConvertToPTimeFromMicroseconds(TRDK_USAGE_STOP_TIMESTAMP_MS * 1000)) {
      return;
    }
    if (context.GetCurrentTime() >=
        ConvertToPTimeFromMicroseconds(TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS *
                                       1000)) {
      const auto &period = TRDK_USAGE_STOP_TIMESTAMP_MS -
                           TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS;
      const auto &now = (context.GetCurrentTime() -
                         ConvertToPTimeFromMicroseconds(
                             TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS * 1000))
                            .total_milliseconds();
      const size_t chance = (now * 100) / period;
      boost::mt19937 random;
      boost::uniform_int<decltype(chance)> range(0, 100);
      boost::variate_generator<boost::mt19937,
                               boost::uniform_int<decltype(chance)>>
          generator(random, range);
      if (generator() <= chance) {
        return;
      }
    }
  }
#endif

  unsigned int threadsCount = 2;
  boost::shared_ptr<boost::barrier> startBarrier(
      new boost::barrier(threadsCount + 1));
  StartNotificationTask<DispatchingTimeMeasurementPolicy>(
      startBarrier, m_level1Updates, m_level1Ticks, m_bookUpdateTicks,
      m_newTrades, m_newBars, threadsCount);
  StartNotificationTask<DispatchingTimeMeasurementPolicy>(
      startBarrier, m_positionsUpdates, m_brokerPositionsUpdates, threadsCount);
  AssertEq(0, threadsCount);
  startBarrier->wait();
}

Dispatcher::~Dispatcher() {
  try {
    m_context.GetLog().Debug("Stopping events dispatching...");
    for (auto &queue : m_queues) {
      boost::apply_visitor(StopVisitor(), queue);
    }
    m_threads.join_all();
    m_context.GetLog().Debug("Events dispatching stopped.");
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void Dispatcher::Activate() {
  m_context.GetLog().Debug("Starting events dispatching...");
  for (auto &queue : m_queues) {
    boost::apply_visitor(ActivateVisitor(), queue);
  }
  m_context.GetLog().Debug("Events dispatching started.");
}

void Dispatcher::Suspend() {
  m_context.GetLog().Debug("Suspending events dispatching...");
  for (auto &queue : m_queues) {
    boost::apply_visitor(SuspendVisitor(), queue);
  }
  m_context.GetLog().Debug("Events dispatching suspended.");
}

Dispatcher::UniqueSyncLock Dispatcher::SyncDispatching() const {
  UniqueSyncLock lock(m_syncMutex);
  for (auto &queue : m_queues) {
    boost::apply_visitor(SyncVisitor(), queue);
  }
  return lock;
}

bool Dispatcher::IsActive() const {
  for (auto &queue : m_queues) {
    if (!boost::apply_visitor(IsActiveVisitor(), queue)) {
      return false;
    }
  }
  return true;
}

void Dispatcher::SignalLevel1Update(
    SubscriberPtrWrapper &subscriber,
    Security &security,
    const TimeMeasurement::Milestones &delayMeasurement) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_level1Updates.Queue(
        boost::make_tuple(&security, subscriber, delayMeasurement), true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalLevel1Tick(
    SubscriberPtrWrapper &subscriber,
    Security &security,
    const boost::posix_time::ptime &time,
    const Level1TickValue &value,
    const TimeMeasurement::Milestones &delayMeasurement,
    bool flush) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_level1Ticks.Queue(
        boost::make_tuple(
            SubscriberPtrWrapper::Level1Tick{&security, time, value},
            subscriber, delayMeasurement),
        flush);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalNewTrade(
    SubscriberPtrWrapper &subscriber,
    Security &security,
    const pt::ptime &time,
    const Price &price,
    const Qty &qty,
    const TimeMeasurement::Milestones &delayMeasurement) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_newTrades.Queue(
        boost::make_tuple(
            SubscriberPtrWrapper::Trade{&security, time, price, qty},
            subscriber, delayMeasurement),
        true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalPositionUpdate(SubscriberPtrWrapper &subscriber,
                                      Position &position) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_positionsUpdates.Queue(
        boost::make_tuple(position.shared_from_this(), subscriber), true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalBrokerPositionUpdate(SubscriberPtrWrapper &subscriber,
                                            Security &security,
                                            bool isLong,
                                            const Qty &qty,
                                            const Volume &volume,
                                            bool isInitial) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_brokerPositionsUpdates.Queue(
        boost::make_tuple(
            SubscriberPtrWrapper::BrokerPosition{&security, isLong, qty, volume,
                                                 isInitial},
            subscriber),
        true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalNewBar(SubscriberPtrWrapper &subscriber,
                              Security &security,
                              const Security::Bar &bar) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_newBars.Queue(boost::make_tuple(&security, bar, subscriber), true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalBookUpdateTick(
    SubscriberPtrWrapper &subscriber,
    Security &security,
    const PriceBook &book,
    const TimeMeasurement::Milestones &timeMeasurement) {
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    m_bookUpdateTicks.Queue(
        boost::make_tuple(&security, book, timeMeasurement, subscriber), true);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalSecurityServiceEvents(
    SubscriberPtrWrapper &subscriber,
    const pt::ptime &time,
    Security &security,
    const Security::ServiceEvent &serviceEvent) {
  Assert(!m_syncMutex.try_lock_shared());
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    subscriber.RaiseSecurityServiceEvent(time, security, serviceEvent);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

void Dispatcher::SignalSecurityContractSwitched(
    SubscriberPtrWrapper &subscriber,
    const pt::ptime &time,
    Security &security,
    Security::Request &request,
    bool &isSwitched) {
  Assert(!m_syncMutex.try_lock_shared());
  try {
    if (subscriber.IsBlocked()) {
      return;
    }
    subscriber.RaiseSecurityContractSwitchedEvent(time, security, request,
                                                  isSwitched);
  } catch (...) {
    AssertFailNoException();
    //! Blocking as irreversible error, data loss.
    subscriber.Block();
    throw;
  }
}

//////////////////////////////////////////////////////////////////////////
