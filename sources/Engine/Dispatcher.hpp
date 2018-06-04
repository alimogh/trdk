/**************************************************************************
 *   Created: 2012/11/22 11:45:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Bar.hpp"
#include "Core/PriceBook.hpp"
#include "Core/Settings.hpp"
#include "Context.hpp"
#include "SubscriberPtrWrapper.hpp"

namespace trdk {
namespace Engine {

////////////////////////////////////////////////////////////////////////////////

namespace Details {

template <Lib::Concurrency::Profile profile>
struct DispatcherConcurrencyPolicyT {
  static_assert(profile == Lib::Concurrency::PROFILE_RELAX,
                "Wrong concurrency profile");

  typedef boost::mutex QueueMutex;
  typedef QueueMutex::scoped_lock QueueLock;
  typedef boost::condition_variable QueueCondition;

  typedef boost::shared_mutex SyncMutex;
  typedef boost::shared_lock<SyncMutex> SharedSyncLock;
  typedef boost::unique_lock<SyncMutex> UniqueSyncLock;
};

template <>
struct DispatcherConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
  typedef Lib::Concurrency::SpinMutex QueueMutex;
  typedef QueueMutex::ScopedLock QueueLock;
  typedef Lib::Concurrency::SpinCondition QueueCondition;
};

typedef DispatcherConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
    DispatcherConcurrencyPolicy;
}  // namespace Details

////////////////////////////////////////////////////////////////////////////////

class Dispatcher : private boost::noncopyable {
 public:
  typedef Details::DispatcherConcurrencyPolicy::UniqueSyncLock UniqueSyncLock;

 private:
  typedef Details::DispatcherConcurrencyPolicy::SharedSyncLock SharedSyncLock;
  typedef Details::DispatcherConcurrencyPolicy::SyncMutex SyncMutex;

  typedef Details::DispatcherConcurrencyPolicy::QueueMutex EventQueueMutex;
  typedef Details::DispatcherConcurrencyPolicy::QueueLock EventQueueLock;
  typedef Details::DispatcherConcurrencyPolicy::QueueCondition
      EventQueueCondition;

  struct EventListsSyncObjects {
    SyncMutex &syncMutex;

    EventQueueMutex queueMutex;
    EventQueueCondition newDataCondition;
    EventQueueCondition syncCondition;
    bool isSyncRequired;

    explicit EventListsSyncObjects(SyncMutex &syncMutex)
        : syncMutex(syncMutex), isSyncRequired(false) {}
  };

  template <typename ListT>
  class EventQueue : private boost::noncopyable {
   public:
    typedef ListT List;

    typedef EventQueueMutex Mutex;
    typedef EventQueueLock Lock;
    typedef EventQueueCondition Condition;

   private:
    enum TaskState {
      TASK_STATE_INACTIVE,
      TASK_STATE_ACTIVE,
      TASK_STATE_STOPPED,
    };

   public:
    explicit EventQueue(const char *name, const Context &context)
        : m_context(context),
          m_name(name),
          m_current(&m_lists.first),
          m_taksState(TASK_STATE_INACTIVE),
          m_queueSizeConstrolLevel(
              !m_context.GetSettings().IsReplayMode() ? 50 : 10000) {}

   public:
    void AssignSyncObjects(boost::shared_ptr<EventListsSyncObjects> &sync) {
      Assert(!m_sync);
      m_sync = sync;
    }

    void Sync() {
      Assert(m_sync);
      Lock lock(m_sync->queueMutex);
      m_sync->isSyncRequired = true;
      m_sync->newDataCondition.notify_one();
      m_sync->syncCondition.wait(lock);
      Assert(!m_sync->isSyncRequired);
    }

    const char *GetName() const { return m_name; }

    bool IsActive() const { return m_taksState == TASK_STATE_ACTIVE; }

    void Activate() {
      Assert(m_sync);
      const Lock lock(m_sync->queueMutex);
      AssertEq(int(TASK_STATE_INACTIVE), int(m_taksState));
      m_taksState = TASK_STATE_ACTIVE;
    }

    void Suspend() {
      Assert(m_sync);
      const Lock lock(m_sync->queueMutex);
      AssertNe(int(TASK_STATE_STOPPED), int(m_taksState));
      m_taksState = TASK_STATE_INACTIVE;
    }

    void Stop() {
      Assert(m_sync);
      const Lock lock(m_sync->queueMutex);
      AssertNe(int(TASK_STATE_STOPPED), int(m_taksState));
      m_taksState = TASK_STATE_STOPPED;
      m_sync->newDataCondition.notify_all();
    }

    bool IsStopped(const Lock &lock) const {
      Assert(m_sync);
      Assert(&m_sync->queueMutex == lock.mutex());
      Lib::UseUnused(lock);
      return m_taksState == TASK_STATE_STOPPED;
    }

    template <typename Event>
    void Queue(Event &&event, bool flush) {
      Assert(m_sync);
      const SharedSyncLock syncLock(m_sync->syncMutex);
      {
        const Lock queueLock(m_sync->queueMutex);
        if (m_taksState == TASK_STATE_STOPPED) {
          return;
        }
        Assert(m_current == &m_lists.first || m_current == &m_lists.second);
        if (!Dispatcher::QueueEvent(std::forward<Event>(event), *m_current)) {
          flush = false;
        }
        if (flush) {
          flush = !m_context.GetSettings().IsReplayMode();
        }
        if (!(m_current->size() % m_queueSizeConstrolLevel)) {
          const auto message =
              "Dispatcher queue \"%1%\" is too long (%2% events)!";
          if (m_current->size() <= 100) {
            m_context.GetLog().Info(message, m_name, m_current->size());
          } else if (m_current->size() <= 200) {
            m_context.GetLog().Warn(message, m_name, m_current->size());
          } else {
            m_context.GetLog().Error(message, m_name, m_current->size());
          }
        }
      }
      if (flush) {
        m_sync->newDataCondition.notify_one();
      }
    }

    bool Flush(Lock &lock,
               const Lib::TimeMeasurement::Milestones &timeMeasurement) {
      Assert(m_sync);
      Assert(&m_sync->queueMutex == lock.mutex());
      Assert(m_current == &m_lists.first || m_current == &m_lists.second);

      size_t heavyLoadsCount = 0;
      while (!m_current->empty() && m_taksState == TASK_STATE_ACTIVE) {
        if (!(++heavyLoadsCount % 500)) {
          m_context.GetLog().Warn(
              "Dispatcher task \"%1%\" is heavy loaded (%2% iterations)!",
              m_name, heavyLoadsCount);
        }

        List *listToRead = m_current;
        m_current =
            m_current == &m_lists.first ? &m_lists.second : &m_lists.first;

        lock.unlock();
        Assert(!listToRead->empty());
        for (auto &event : *listToRead) {
          Dispatcher::RaiseEvent(event);
        }
        listToRead->clear();
        lock.lock();
        timeMeasurement.Measure(Lib::TimeMeasurement::DM_COMPLETE_LIST);
      }

      if (m_sync->isSyncRequired) {
        m_sync->isSyncRequired = false;
        m_sync->syncCondition.notify_all();
      }

      return heavyLoadsCount > 0;
    }

   private:
    const Context &m_context;

    const char *const m_name;

    std::pair<List, List> m_lists;
    List *m_current;

    boost::shared_ptr<EventListsSyncObjects> m_sync;

    TaskState m_taksState;

    const size_t m_queueSizeConstrolLevel;
  };

  typedef boost::
      tuple<Security *, SubscriberPtrWrapper, Lib::TimeMeasurement::Milestones>
          Level1UpdateEvent;
  typedef EventQueue<std::vector<Level1UpdateEvent>> Level1UpdateEventQueue;

  typedef boost::tuple<SubscriberPtrWrapper::Level1Tick,
                       SubscriberPtrWrapper,
                       Lib::TimeMeasurement::Milestones>
      Level1TickEvent;
  typedef EventQueue<std::vector<Level1TickEvent>> Level1TicksEventQueue;

  typedef boost::tuple<SubscriberPtrWrapper::Trade,
                       SubscriberPtrWrapper,
                       Lib::TimeMeasurement::Milestones>
      NewTradeEvent;
  typedef EventQueue<std::vector<NewTradeEvent>> NewTradeEventQueue;

  //! Increasing position references count for safety work in core (which
  //! can lost owning references at event handling).
  typedef boost::tuple<boost::shared_ptr<Position>, SubscriberPtrWrapper>
      PositionUpdateEvent;
  //! @todo: Check performance: set or list
  typedef EventQueue<std::vector<PositionUpdateEvent>>
      PositionsUpdateEventQueue;

  typedef boost::tuple<SubscriberPtrWrapper::BrokerPosition,
                       SubscriberPtrWrapper>
      BrokerPositionUpdateEvent;
  //! @todo: Check performance: map to exclude duplicate securities
  typedef EventQueue<std::vector<BrokerPositionUpdateEvent>>
      BrokerPositionsUpdateEventQueue;

  typedef boost::tuple<Security *, Bar, SubscriberPtrWrapper> NewBarEvent;
  //! @todo HAVY OPTIMIZATION!!! Use preallocated buffer here instead std::list.
  typedef EventQueue<std::vector<NewBarEvent>> NewBarEventQueue;

  typedef boost::tuple<Security *,
                       PriceBook,
                       Lib::TimeMeasurement::Milestones,
                       SubscriberPtrWrapper>
      BookUpdateTickEvent;
  //! @todo HAVY OPTIMIZATION!!! Use preallocated buffer here instead std::list.
  typedef EventQueue<std::vector<BookUpdateTickEvent>> BookUpdateTickEventQueue;

  typedef boost::variant<Level1UpdateEventQueue *,
                         Level1TicksEventQueue *,
                         NewTradeEventQueue *,
                         PositionsUpdateEventQueue *,
                         BrokerPositionsUpdateEventQueue *,
                         NewBarEventQueue *,
                         BookUpdateTickEventQueue *>
      QueueList;

 public:
  explicit Dispatcher(Engine::Context &);
  ~Dispatcher();

  bool IsActive() const;

  void Activate();
  void Suspend();

  UniqueSyncLock SyncDispatching() const;

  void SignalSecurityContractSwitched(SubscriberPtrWrapper &,
                                      const boost::posix_time::ptime &,
                                      Security &,
                                      Security::Request &,
                                      bool &isSwitched);
  void SignalLevel1Update(SubscriberPtrWrapper &,
                          Security &,
                          const Lib::TimeMeasurement::Milestones &);
  void SignalLevel1Tick(SubscriberPtrWrapper &,
                        Security &,
                        const boost::posix_time::ptime &,
                        const trdk::Level1TickValue &,
                        const trdk::Lib::TimeMeasurement::Milestones &,
                        bool flush);
  void SignalNewTrade(SubscriberPtrWrapper &,
                      Security &,
                      const boost::posix_time::ptime &,
                      const Price &,
                      const Qty &,
                      const trdk::Lib::TimeMeasurement::Milestones &);
  void SignalPositionUpdate(SubscriberPtrWrapper &, Position &);
  void SignalBrokerPositionUpdate(SubscriberPtrWrapper &,
                                  Security &,
                                  bool isLong,
                                  const Qty &,
                                  const Volume &,
                                  bool isInitial);
  void SignalNewBar(SubscriberPtrWrapper &, Security &, const Bar &);
  void SignalBookUpdateTick(SubscriberPtrWrapper &,
                            Security &,
                            const PriceBook &,
                            const Lib::TimeMeasurement::Milestones &);
  void SignalSecurityServiceEvents(SubscriberPtrWrapper &,
                                   const boost::posix_time::ptime &,
                                   Security &,
                                   const Security::ServiceEvent &);

 private:
  template <typename Event>
  static void RaiseEvent(Event &) {
#if !defined(__GNUG__)
    static_assert(false, "Failed to find event raise specialization.");
#else
    AssertFail("Failed to find event raise specialization.");
#endif
  }

  template <typename Event, typename EventList>
  static bool QueueEvent(Event &&, EventList &) {
#if !defined(__GNUG__)
    static_assert(false, "Failed to find event queue specialization.");
#else
    AssertFail("Failed to find event raise specialization.");
#endif
    return false;
  }

  template <typename EventList>
  static bool QueueEvent(Level1UpdateEvent &&updateEvent,
                         EventList &eventList) {
    //! @todo place for optimization
    for (const auto &it : boost::adaptors::reverse(eventList)) {
      if (boost::get<0>(updateEvent) == boost::get<0>(it) &&
          boost::get<1>(updateEvent) == boost::get<1>(it)) {
        return false;
      }
    }
    eventList.emplace_back(std::move(updateEvent));
    boost::get<2>(eventList.back())
        .Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_ENQUEUE);
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(Level1TickEvent &&tick, EventList &eventList) {
    eventList.emplace_back(std::move(tick));
    boost::get<2>(eventList.back())
        .Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_ENQUEUE);
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(NewTradeEvent &&newTradeEvent, EventList &eventList) {
    eventList.emplace_back(std::move(newTradeEvent));
    boost::get<2>(eventList.back())
        .Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_ENQUEUE);
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(PositionUpdateEvent &&positionUpdateEvent,
                         EventList &eventList) {
    //! @todo place for optimization
    const auto &end = eventList.cend();
    const auto &pos = std::find(eventList.cbegin(), end, positionUpdateEvent);
    if (pos != end) {
      return false;
    }
    eventList.emplace_back(std::move(positionUpdateEvent));
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(BrokerPositionUpdateEvent &&positionUpdateEvent,
                         EventList &eventList) {
    eventList.emplace_back(std::move(positionUpdateEvent));
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(NewBarEvent &&newBarEvent, EventList &eventList) {
    eventList.emplace_back(newBarEvent);
    return true;
  }
  template <typename EventList>
  static bool QueueEvent(BookUpdateTickEvent &&bookUpdateTickEvent,
                         EventList &eventList) {
    eventList.emplace_back(bookUpdateTickEvent);
    boost::get<2>(eventList.back())
        .Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_ENQUEUE);
    return true;
  }

 private:
  ////////////////////////////////////////////////////////////////////////////////

  template <typename EventList>
  bool FlushEvents(
      EventList &list,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    try {
      return list.Flush(lock, timeMeasurement);
    } catch (const trdk::Lib::Exception &ex) {
      m_context.GetLog().Error(
          R"(Error in dispatcher notification task "%1%": "%2%".)",
          list.GetName(), ex);
      throw;
    } catch (...) {
      m_context.GetLog().Error(
          "Unhandled exception caught in dispatcher notification task \"%1%\".",
          list.GetName());
      AssertFailNoException();
      throw;
    }
  }

  template <size_t index, typename EventLists>
  bool FlushEventList(
      EventLists &lists,
      std::bitset<boost::tuples::length<EventLists>::value> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    if (deactivationMask[index]) {
      return false;
    }
    auto &list = boost::get<index>(lists);
    if (FlushEvents(list, lock, timeMeasurement)) {
      return true;
    }
    deactivationMask[index] = list.IsStopped(lock);
    return false;
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy, typename EventList>
  void StartNotificationTask(boost::shared_ptr<boost::barrier> startBarrier,
                             EventList &list,
                             unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(list));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1>
  static std::string GetEventListsName(const boost::tuple<T1> &lists) {
    return boost::get<0>(lists).GetName();
  }

  template <typename T1>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
  }

  template <typename T1>
  void FlushEventListsCollection(
      const boost::tuple<T1> &lists,
      std::bitset<1> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority>
  void StartNotificationTask(boost::shared_ptr<boost::barrier> startBarrier,
                             ListWithHighPriority &listWithHighPriority,
                             ListWithLowPriority &listWithLowPriority,
                             unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1, typename T2>
  static std::string GetEventListsName(const boost::tuple<T1, T2> &lists) {
    boost::format result("%1%, %2%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName();
    return result.str();
  }

  template <typename T1, typename T2>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
  }

  template <typename T1, typename T2>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2> &lists,
      std::bitset<2> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
    } while (FlushEventList<1>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority>
  void StartNotificationTask(boost::shared_ptr<boost::barrier> startBarrier,
                             ListWithHighPriority &listWithHighPriority,
                             ListWithLowPriority &listWithLowPriority,
                             ListWithExtraLowPriority &listWithExtraLowPriority,
                             unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1, typename T2, typename T3>
  static std::string GetEventListsName(const boost::tuple<T1, T2, T3> &lists) {
    boost::format result("%1%, %2%, %3%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName();
    return result.str();
  }

  template <typename T1, typename T2, typename T3>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
  }

  template <typename T1, typename T2, typename T3>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3> &lists,
      std::bitset<3> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
      } while (
          FlushEventList<1>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<2>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority,
            typename ListWithExtraLowPriority2>
  void StartNotificationTask(
      boost::shared_ptr<boost::barrier> startBarrier,
      ListWithHighPriority &listWithHighPriority,
      ListWithLowPriority &listWithLowPriority,
      ListWithExtraLowPriority &listWithExtraLowPriority,
      ListWithExtraLowPriority2 &listWithExtraLowPriority2,
      unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority),
                                         boost::ref(listWithExtraLowPriority2));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1, typename T2, typename T3, typename T4>
  static std::string GetEventListsName(
      const boost::tuple<T1, T2, T3, T4> &lists) {
    boost::format result("%1%, %2%, %3%, %4%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName() % boost::get<3>(lists).GetName();
    return result.str();
  }

  template <typename T1, typename T2, typename T3, typename T4>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3, T4> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
    boost::get<3>(lists).AssignSyncObjects(sync);
  }

  template <typename T1, typename T2, typename T3, typename T4>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3, T4> &lists,
      std::bitset<4> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        do {
          FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
        } while (
            FlushEventList<1>(lists, deactivationMask, lock, timeMeasurement));
      } while (
          FlushEventList<2>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<3>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority,
            typename ListWithExtraLowPriority2,
            typename ListWithExtraLowPriority3>
  void StartNotificationTask(
      boost::shared_ptr<boost::barrier> startBarrier,
      ListWithHighPriority &listWithHighPriority,
      ListWithLowPriority &listWithLowPriority,
      ListWithExtraLowPriority &listWithExtraLowPriority,
      ListWithExtraLowPriority2 &listWithExtraLowPriority2,
      ListWithExtraLowPriority3 &listWithExtraLowPriority3,
      unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority),
                                         boost::ref(listWithExtraLowPriority2),
                                         boost::ref(listWithExtraLowPriority3));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  static std::string GetEventListsName(
      const boost::tuple<T1, T2, T3, T4, T5> &lists) {
    boost::format result("%1%, %2%, %3%, %4%, %5%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName() % boost::get<3>(lists).GetName() %
        boost::get<4>(lists).GetName();
    return result.str();
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3, T4, T5> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
    boost::get<3>(lists).AssignSyncObjects(sync);
    boost::get<4>(lists).AssignSyncObjects(sync);
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3, T4, T5> &lists,
      std::bitset<5> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        do {
          do {
            FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
          } while (FlushEventList<1>(lists, deactivationMask, lock,
                                     timeMeasurement));
        } while (
            FlushEventList<2>(lists, deactivationMask, lock, timeMeasurement));
      } while (
          FlushEventList<3>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<4>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority,
            typename ListWithExtraLowPriority2,
            typename ListWithExtraLowPriority3,
            typename ListWithExtraLowPriority4>
  void StartNotificationTask(
      boost::shared_ptr<boost::barrier> startBarrier,
      ListWithHighPriority &listWithHighPriority,
      ListWithLowPriority &listWithLowPriority,
      ListWithExtraLowPriority &listWithExtraLowPriority,
      ListWithExtraLowPriority2 &listWithExtraLowPriority2,
      ListWithExtraLowPriority3 &listWithExtraLowPriority3,
      ListWithExtraLowPriority4 &listWithExtraLowPriority4,
      unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority),
                                         boost::ref(listWithExtraLowPriority2),
                                         boost::ref(listWithExtraLowPriority3),
                                         boost::ref(listWithExtraLowPriority4));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6>
  static std::string GetEventListsName(
      const boost::tuple<T1, T2, T3, T4, T5, T6> &lists) {
    boost::format result("%1%, %2%, %3%, %4%, %5%, %6%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName() % boost::get<3>(lists).GetName() %
        boost::get<4>(lists).GetName() % boost::get<5>(lists).GetName();
    return result.str();
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3, T4, T5, T6> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
    boost::get<3>(lists).AssignSyncObjects(sync);
    boost::get<4>(lists).AssignSyncObjects(sync);
    boost::get<5>(lists).AssignSyncObjects(sync);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3, T4, T5, T6> &lists,
      std::bitset<6> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        do {
          do {
            do {
              FlushEventList<0>(lists, deactivationMask, lock, timeMeasurement);
            } while (FlushEventList<1>(lists, deactivationMask, lock,
                                       timeMeasurement));
          } while (FlushEventList<2>(lists, deactivationMask, lock,
                                     timeMeasurement));
        } while (
            FlushEventList<3>(lists, deactivationMask, lock, timeMeasurement));
      } while (
          FlushEventList<4>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<5>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority,
            typename ListWithExtraLowPriority2,
            typename ListWithExtraLowPriority3,
            typename ListWithExtraLowPriority4,
            typename ListWithExtraLowPriority5>
  void StartNotificationTask(
      boost::shared_ptr<boost::barrier> startBarrier,
      ListWithHighPriority &listWithHighPriority,
      ListWithLowPriority &listWithLowPriority,
      ListWithExtraLowPriority &listWithExtraLowPriority,
      ListWithExtraLowPriority2 &listWithExtraLowPriority2,
      ListWithExtraLowPriority3 &listWithExtraLowPriority3,
      ListWithExtraLowPriority4 &listWithExtraLowPriority4,
      ListWithExtraLowPriority5 &listWithExtraLowPriority5,
      unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority),
                                         boost::ref(listWithExtraLowPriority2),
                                         boost::ref(listWithExtraLowPriority3),
                                         boost::ref(listWithExtraLowPriority4),
                                         boost::ref(listWithExtraLowPriority5));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7>
  static std::string GetEventListsName(
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7> &lists) {
    boost::format result("%1%, %2%, %3%, %4%, %5%, %6%, %7%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName() % boost::get<3>(lists).GetName() %
        boost::get<4>(lists).GetName() % boost::get<5>(lists).GetName() %
        boost::get<6>(lists).GetName();
    return result.str();
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
    boost::get<3>(lists).AssignSyncObjects(sync);
    boost::get<4>(lists).AssignSyncObjects(sync);
    boost::get<5>(lists).AssignSyncObjects(sync);
    boost::get<6>(lists).AssignSyncObjects(sync);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7> &lists,
      std::bitset<7> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        do {
          do {
            do {
              do {
                FlushEventList<0>(lists, deactivationMask, lock,
                                  timeMeasurement);
              } while (FlushEventList<1>(lists, deactivationMask, lock,
                                         timeMeasurement));
            } while (FlushEventList<2>(lists, deactivationMask, lock,
                                       timeMeasurement));
          } while (FlushEventList<3>(lists, deactivationMask, lock,
                                     timeMeasurement));
        } while (
            FlushEventList<4>(lists, deactivationMask, lock, timeMeasurement));
      } while (
          FlushEventList<5>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<6>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename DispatchingTimeMeasurementPolicy,
            typename ListWithHighPriority,
            typename ListWithLowPriority,
            typename ListWithExtraLowPriority,
            typename ListWithExtraLowPriority2,
            typename ListWithExtraLowPriority3,
            typename ListWithExtraLowPriority4,
            typename ListWithExtraLowPriority5,
            typename ListWithExtraLowPriority6>
  void StartNotificationTask(
      boost::shared_ptr<boost::barrier> startBarrier,
      ListWithHighPriority &listWithHighPriority,
      ListWithLowPriority &listWithLowPriority,
      ListWithExtraLowPriority &listWithExtraLowPriority,
      ListWithExtraLowPriority2 &listWithExtraLowPriority2,
      ListWithExtraLowPriority3 &listWithExtraLowPriority3,
      ListWithExtraLowPriority4 &listWithExtraLowPriority4,
      ListWithExtraLowPriority5 &listWithExtraLowPriority5,
      ListWithExtraLowPriority6 &listWithExtraLowPriority6,
      unsigned int &threadsCounter) {
    Lib::UseUnused(threadsCounter);
    const auto lists = boost::make_tuple(boost::ref(listWithHighPriority),
                                         boost::ref(listWithLowPriority),
                                         boost::ref(listWithExtraLowPriority),
                                         boost::ref(listWithExtraLowPriority2),
                                         boost::ref(listWithExtraLowPriority3),
                                         boost::ref(listWithExtraLowPriority4),
                                         boost::ref(listWithExtraLowPriority5),
                                         boost::ref(listWithExtraLowPriority6));
    m_threads.create_thread(boost::bind(
        &Dispatcher::NotificationTask<decltype(lists),
                                      DispatchingTimeMeasurementPolicy>,
        this, startBarrier, lists));
    Assert(1 <= threadsCounter--);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7,
            typename T8>
  static std::string GetEventListsName(
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8> &lists) {
    boost::format result("%1%, %2%, %3%, %4%, %5%, %6%, %7%, %8%");
    result % boost::get<0>(lists).GetName() % boost::get<1>(lists).GetName() %
        boost::get<2>(lists).GetName() % boost::get<3>(lists).GetName() %
        boost::get<4>(lists).GetName() % boost::get<5>(lists).GetName() %
        boost::get<6>(lists).GetName() % boost::get<7>(lists).GetName();
    return result.str();
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7,
            typename T8>
  static void AssignEventListsSyncObjects(
      boost::shared_ptr<EventListsSyncObjects> &sync,
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8> &lists) {
    boost::get<0>(lists).AssignSyncObjects(sync);
    boost::get<1>(lists).AssignSyncObjects(sync);
    boost::get<2>(lists).AssignSyncObjects(sync);
    boost::get<3>(lists).AssignSyncObjects(sync);
    boost::get<4>(lists).AssignSyncObjects(sync);
    boost::get<5>(lists).AssignSyncObjects(sync);
    boost::get<6>(lists).AssignSyncObjects(sync);
    boost::get<7>(lists).AssignSyncObjects(sync);
  }

  template <typename T1,
            typename T2,
            typename T3,
            typename T4,
            typename T5,
            typename T6,
            typename T7,
            typename T8>
  void FlushEventListsCollection(
      const boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8> &lists,
      std::bitset<8> &deactivationMask,
      EventQueueLock &lock,
      const Lib::TimeMeasurement::Milestones &timeMeasurement) const {
    do {
      do {
        do {
          do {
            do {
              do {
                do {
                  FlushEventList<0>(lists, deactivationMask, lock,
                                    timeMeasurement);
                } while (FlushEventList<1>(lists, deactivationMask, lock,
                                           timeMeasurement));
              } while (FlushEventList<2>(lists, deactivationMask, lock,
                                         timeMeasurement));
            } while (FlushEventList<3>(lists, deactivationMask, lock,
                                       timeMeasurement));
          } while (FlushEventList<4>(lists, deactivationMask, lock,
                                     timeMeasurement));
        } while (
            FlushEventList<5>(lists, deactivationMask, lock, timeMeasurement));
      } while (
          FlushEventList<6>(lists, deactivationMask, lock, timeMeasurement));
    } while (FlushEventList<7>(lists, deactivationMask, lock, timeMeasurement));
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename EventLists, typename DispatchingTimeMeasurementPolicy>
  void NotificationTask(boost::shared_ptr<boost::barrier> startBarrier,
                        EventLists &lists) const {
    m_context.GetLog().Debug("Dispatcher notification task \"%1%\" started...",
                             GetEventListsName(lists));

    std::string error;

    try {
      std::bitset<boost::tuples::length<EventLists>::value> deactivationMask;
      auto sync = boost::make_shared<EventListsSyncObjects>(m_syncMutex);
      AssignEventListsSyncObjects(sync, lists);
      startBarrier->wait();
      startBarrier.reset();
      EventQueueLock lock(sync->queueMutex);
      for (;;) {
        const Lib::TimeMeasurement::Milestones timeMeasurement =
            DispatchingTimeMeasurementPolicy::StartDispatchingTimeMeasurement(
                m_context);
        FlushEventListsCollection(lists, deactivationMask, lock,
                                  timeMeasurement);
        timeMeasurement.Measure(Lib::TimeMeasurement::DM_COMPLETE_ALL);
        if (deactivationMask.all()) {
          break;
        }
        sync->newDataCondition.wait(lock);
        timeMeasurement.Measure(Lib::TimeMeasurement::DM_NEW_DATA);
      }

    } catch (const trdk::Lib::Exception &ex) {
      // error already logged
      error = ex.what();
    } catch (...) {
      // error already logged
      AssertFailNoException();
      error = "Unknown error";
    }

    try {
      if (error.empty()) {
        m_context.RaiseStateUpdate(
            Context::STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY);
      } else {
        m_context.RaiseStateUpdate(Context::STATE_DISPATCHER_TASK_STOPPED_ERROR,
                                   error);
      }
    } catch (const trdk::Lib::Exception &ex) {
      m_context.GetLog().Error(
          "Error context at state update notification \"%1%\".", ex);
    } catch (...) {
      AssertFailNoException();
      throw;
    }

    m_context.GetLog().Debug("Dispatcher notification task \"%1%\" stopped.",
                             GetEventListsName(lists));

    if (startBarrier) {
      startBarrier->wait();
    }
  }

 private:
  Engine::Context &m_context;

  mutable SyncMutex m_syncMutex;

  boost::thread_group m_threads;

  Level1UpdateEventQueue m_level1Updates;
  Level1TicksEventQueue m_level1Ticks;
  NewTradeEventQueue m_newTrades;
  PositionsUpdateEventQueue m_positionsUpdates;
  BrokerPositionsUpdateEventQueue m_brokerPositionsUpdates;
  NewBarEventQueue m_newBars;
  BookUpdateTickEventQueue m_bookUpdateTicks;

  std::vector<QueueList> m_queues;
};

////////////////////////////////////////////////////////////////////////////////

template <>
inline void Dispatcher::RaiseEvent(Level1UpdateEvent &level1Update) {
  Lib::TimeMeasurement::Milestones &timeMeasurement =
      boost::get<2>(level1Update);
  timeMeasurement.Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_DEQUEUE);
  boost::get<1>(level1Update)
      .RaiseLevel1UpdateEvent(*boost::get<0>(level1Update), timeMeasurement);
}
template <>
inline void Dispatcher::RaiseEvent(Level1TickEvent &tick) {
  boost::get<1>(tick).RaiseLevel1TickEvent(boost::get<0>(tick),
                                           Lib::TimeMeasurement::Milestones());
}
template <>
inline void Dispatcher::RaiseEvent(NewTradeEvent &newTradeEvent) {
  boost::get<1>(newTradeEvent)
      .RaiseNewTradeEvent(boost::get<0>(newTradeEvent),
                          Lib::TimeMeasurement::Milestones());
}
template <>
inline void Dispatcher::RaiseEvent(PositionUpdateEvent &positionUpdateEvent) {
  boost::get<1>(positionUpdateEvent)
      .RaisePositionUpdateEvent(*boost::get<0>(positionUpdateEvent));
}
template <>
inline void Dispatcher::RaiseEvent(
    BrokerPositionUpdateEvent &positionUpdateEvent) {
  boost::get<1>(positionUpdateEvent)
      .RaiseBrokerPositionUpdateEvent(boost::get<0>(positionUpdateEvent),
                                      Lib::TimeMeasurement::Milestones());
}
template <>
inline void Dispatcher::RaiseEvent(NewBarEvent &newBarEvent) {
  boost::get<2>(newBarEvent)
      .RaiseNewBarEvent(*boost::get<0>(newBarEvent), boost::get<1>(newBarEvent),
                        Lib::TimeMeasurement::Milestones());
}
template <>
inline void Dispatcher::RaiseEvent(BookUpdateTickEvent &bookUpdateTickEvent) {
  auto &timeMeasurement = boost::get<2>(bookUpdateTickEvent);
  timeMeasurement.Measure(Lib::TimeMeasurement::SM_DISPATCHING_DATA_DEQUEUE);
  boost::get<3>(bookUpdateTickEvent)
      .RaiseBookUpdateTickEvent(*boost::get<0>(bookUpdateTickEvent),
                                boost::get<1>(bookUpdateTickEvent),
                                timeMeasurement);
}

////////////////////////////////////////////////////////////////////////////////
}  // namespace Engine
}  // namespace trdk
