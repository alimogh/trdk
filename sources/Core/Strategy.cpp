/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "DropCopy.hpp"
#include "Operation.hpp"
#include "RiskControl.hpp"
#include "Service.hpp"
#include "Settings.hpp"
#ifndef BOOST_WINDOWS
#include <signal.h>
#endif

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;
namespace uuids = boost::uuids;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

namespace {

class PositionHolder {
 public:
  explicit PositionHolder(
      Position &position,
      const Position::StateUpdateConnection &stateUpdateConnection)
      : m_refCount(new size_t(1)),
        m_position(position.shared_from_this()),
        m_stateUpdateConnection(stateUpdateConnection) {
    Assert(m_stateUpdateConnection.connected());
  }

  PositionHolder(const PositionHolder &rhs)
      : m_refCount(rhs.m_refCount),
        m_position(rhs.m_position),
        m_stateUpdateConnection(rhs.m_stateUpdateConnection) {
    Verify(++*m_refCount > 1);
  }

  ~PositionHolder() {
    Assert(m_stateUpdateConnection.connected());
    AssertLt(0, *m_refCount);
    if (!--*m_refCount) {
      try {
        m_stateUpdateConnection.disconnect();
      } catch (...) {
        AssertFailNoException();
        terminate();
      }
      delete m_refCount;
    }
  }

 private:
  PositionHolder &operator=(const PositionHolder &);

 public:
  bool operator==(const Position &rhs) const { return GetPtr() == &rhs; }

  Position &operator*() const {
    return const_cast<PositionHolder *>(this)->Get();
  }

  Position *operator->() const {
    return const_cast<PositionHolder *>(this)->GetPtr();
  }

  Position &Get() { return *m_position; }
  const Position &Get() const {
    return const_cast<PositionHolder *>(this)->Get();
  }
  Position *GetPtr() { return &Get(); }
  const Position *GetPtr() const {
    return const_cast<PositionHolder *>(this)->GetPtr();
  }

 private:
  size_t *m_refCount;
  boost::shared_ptr<Position> m_position;
  Position::StateUpdateConnection m_stateUpdateConnection;
};

struct ByPtr {};

typedef boost::multi_index_container<
    PositionHolder,
    mi::indexed_by<
        mi::ordered_unique<mi::tag<ByPtr>,
                           mi::const_mem_fun<PositionHolder,
                                             const Position *,
                                             &PositionHolder::GetPtr>>>>
    PositionHolderList;
}

class Strategy::PositionList::Iterator::Implementation {
 public:
  PositionHolderList::iterator iterator;

 public:
  explicit Implementation(PositionHolderList::iterator iterator)
      : iterator(iterator) {}
};

class Strategy::PositionList::ConstIterator::Implementation {
 public:
  PositionHolderList::const_iterator iterator;

 public:
  explicit Implementation(PositionHolderList::const_iterator iterator)
      : iterator(iterator) {}
};

Strategy::PositionList::Iterator::Iterator(Implementation *pimpl) noexcept
    : m_pimpl(pimpl) {
  Assert(m_pimpl);
}
Strategy::PositionList::Iterator::Iterator(const Iterator &rhs)
    : m_pimpl(new Implementation(*rhs.m_pimpl)) {}
Strategy::PositionList::Iterator::~Iterator() { delete m_pimpl; }
Strategy::PositionList::Iterator &Strategy::PositionList::Iterator::operator=(
    const Iterator &rhs) {
  Assert(this != &rhs);
  Iterator(rhs).Swap(*this);
  return *this;
}
void Strategy::PositionList::Iterator::Swap(Iterator &rhs) {
  Assert(this != &rhs);
  std::swap(m_pimpl, rhs.m_pimpl);
}
Position &Strategy::PositionList::Iterator::dereference() const {
  return **m_pimpl->iterator;
}
bool Strategy::PositionList::Iterator::equal(const Iterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
bool Strategy::PositionList::Iterator::equal(const ConstIterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
void Strategy::PositionList::Iterator::increment() { ++m_pimpl->iterator; }
void Strategy::PositionList::Iterator::decrement() { --m_pimpl->iterator; }
void Strategy::PositionList::Iterator::advance(const difference_type &n) {
  std::advance(m_pimpl->iterator, n);
}

Strategy::PositionList::ConstIterator::ConstIterator(
    Implementation *pimpl) noexcept
    : m_pimpl(pimpl) {
  Assert(m_pimpl);
}
Strategy::PositionList::ConstIterator::ConstIterator(
    const Iterator &rhs) noexcept
    : m_pimpl(new Implementation(rhs.m_pimpl->iterator)) {}
Strategy::PositionList::ConstIterator::ConstIterator(const ConstIterator &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}
Strategy::PositionList::ConstIterator::~ConstIterator() = default;
Strategy::PositionList::ConstIterator &Strategy::PositionList::ConstIterator::
operator=(const ConstIterator &rhs) {
  Assert(this != &rhs);
  ConstIterator(rhs).Swap(*this);
  return *this;
}
void Strategy::PositionList::ConstIterator::Swap(ConstIterator &rhs) {
  Assert(this != &rhs);
  std::swap(m_pimpl, rhs.m_pimpl);
}
const Position &Strategy::PositionList::ConstIterator::dereference() const {
  return **m_pimpl->iterator;
}
bool Strategy::PositionList::ConstIterator::equal(
    const ConstIterator &rhs) const {
  Assert(this != &rhs);
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
bool Strategy::PositionList::ConstIterator::equal(const Iterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}
void Strategy::PositionList::ConstIterator::increment() { ++m_pimpl->iterator; }
void Strategy::PositionList::ConstIterator::decrement() { --m_pimpl->iterator; }
void Strategy::PositionList::ConstIterator::advance(const difference_type &n) {
  std::advance(m_pimpl->iterator, n);
}

//////////////////////////////////////////////////////////////////////////

class Strategy::Implementation : private boost::noncopyable {
 public:
  typedef boost::mutex BlockMutex;
  typedef BlockMutex::scoped_lock BlockLock;
  typedef boost::condition_variable StopCondition;

  class PositionList : public Strategy::PositionList {
   public:
    virtual ~PositionList() override = default;

   public:
    void Insert(const PositionHolder &&holder) {
      Verify(m_impl.emplace(std::move(holder)).second);
    }

    void Erase(const Position &position) {
      AssertLt(0, m_impl.get<ByPtr>().count(&position));
      m_impl.get<ByPtr>().erase(&position);
    }

    bool Has(const Position &position) const {
      return m_impl.get<ByPtr>().count(&position) > 0;
    }

   public:
    virtual size_t GetSize() const { return m_impl.size(); }

    virtual bool IsEmpty() const { return m_impl.empty(); }

    virtual Iterator GetBegin() {
      return Iterator(new Iterator::Implementation(m_impl.begin()));
    }
    virtual ConstIterator GetBegin() const {
      return ConstIterator(new ConstIterator::Implementation(m_impl.begin()));
    }
    virtual Iterator GetEnd() {
      return Iterator(new Iterator::Implementation(m_impl.end()));
    }
    virtual ConstIterator GetEnd() const {
      return ConstIterator(new ConstIterator::Implementation(m_impl.end()));
    }

   private:
    PositionHolderList m_impl;
  };

  class ThreadPositionListTransaction : public PositionListTransaction {
   public:
    class Data : private boost::noncopyable {
     public:
      explicit Data(PositionList &list) : m_originalList(list) {}

      ~Data() {
        try {
          const auto &end = m_inserted.cend();
          for (auto i = m_inserted.begin(); i != end; ++i) {
            m_originalList.Insert(std::move(*i));
          }
        } catch (...) {
          AssertFailNoException();
          terminate();
        }
      }

     public:
      void Insert(PositionHolder &&position) {
        Assert(std::find(m_inserted.cbegin(), m_inserted.cend(), *position) ==
               m_inserted.cend());
        m_inserted.emplace_back(std::move(position));
      }

      void Erase(const Position &position) {
        // Supported only current list as another was not required before.
        const auto it =
            std::find(m_inserted.begin(), m_inserted.end(), position);
        Assert(it != m_inserted.cend());
        if (it == m_inserted.cend()) {
          return;
        }
        m_inserted.erase(it);
      }

     private:
      PositionList &m_originalList;
      std::list<PositionHolder> m_inserted;
    };

   public:
    explicit ThreadPositionListTransaction(PositionList &list) {
      if (IsStarted()) {
        throw SystemException(
            "Thread position list transaction already started");
      }
      m_instance.reset(new Data(list));
    }
    virtual ~ThreadPositionListTransaction() override {
      Assert(IsStarted());
      Assert(m_instance.get());
      m_instance.reset();
    }

   public:
    static bool IsStarted() { return m_instance.get() ? true : false; }
    static Data &GetData() {
      Assert(m_instance.get());
      Assert(IsStarted());
      return *m_instance.get();
    }

   private:
    static boost::thread_specific_ptr<Data> m_instance;
  };

  template <typename SlotSignature>
  struct SignalTrait {
    typedef sig::signal<
        SlotSignature,
        sig::optional_last_value<
            typename boost::function_traits<SlotSignature>::result_type>,
        int,
        std::less<int>,
        boost::function<SlotSignature>,
        typename sig::detail::extended_signature<
            boost::function_traits<SlotSignature>::arity,
            SlotSignature>::function_type,
        sig::dummy_mutex>
        Signal;
  };

 public:
  Strategy &m_strategy;
  const uuids::uuid m_typeId;
  const std::string m_title;
  const TradingMode m_tradingMode;

  const std::unique_ptr<RiskControlScope> m_riskControlScope;

  boost::atomic_bool m_isEnabled;

  boost::atomic_bool m_isBlocked;
  pt::ptime m_blockEndTime;
  BlockMutex m_blockMutex;
  StopCondition m_stopCondition;
  StopMode m_stopMode;

  PositionList m_positions;
  SignalTrait<PositionUpdateSlotSignature>::Signal m_positionUpdateSignal;

  std::vector<Position *> m_delayedPositionToForget;

  DropCopyStrategyInstanceId m_dropCopyInstanceId;

 public:
  explicit Implementation(Strategy &strategy,
                          const std::string &typeUuid,
                          const IniSectionRef &conf)
      : m_strategy(strategy),
        m_typeId(boost::uuids::string_generator()(typeUuid)),
        m_title(conf.ReadKey("title")),
        m_tradingMode(
            ConvertTradingModeFromString(conf.ReadKey("trading_mode"))),
        m_isEnabled(conf.ReadBoolKey("is_enabled")),
        m_isBlocked(false),
        m_riskControlScope(
            m_strategy.GetContext()
                .GetRiskControl(m_tradingMode)
                .CreateScope(m_strategy.GetInstanceName(), conf)),
        m_stopMode(STOP_MODE_UNKNOWN),
        m_dropCopyInstanceId(DropCopy::nStrategyInstanceId) {}

 public:
  void ForgetPosition(const Position &position) {
    Assert(std::find(m_delayedPositionToForget.cbegin(),
                     m_delayedPositionToForget.cend(),
                     &position) == m_delayedPositionToForget.cend());
    ThreadPositionListTransaction::IsStarted()
        ? ThreadPositionListTransaction::GetData().Erase(position)
        : m_positions.Erase(position);
  }

  void BlockByRiskControlEvent(const RiskControlException &ex,
                               const char *action) const {
    boost::format message("Risk Control event: \"%1%\" (at %2%).");
    message % ex % action;
    m_strategy.Block(message.str());
  }

  void Block(const std::string *reason = nullptr) noexcept {
    try {
      const BlockLock lock(m_blockMutex);
      m_isBlocked = true;
      m_blockEndTime = pt::not_a_date_time;
      reason ? m_strategy.GetLog().Info("Blocked by reason: \"%s\".", *reason)
             : m_strategy.GetLog().Info("Blocked.");
      m_stopCondition.notify_all();
    } catch (...) {
      AssertFailNoException();
      raise(SIGTERM);  // is it can mutex or notify_all, also see "include"
    }
    try {
      reason
          ? m_strategy.GetContext().RaiseStateUpdate(
                Context::STATE_STRATEGY_BLOCKED, *reason)
          : m_strategy.GetContext().RaiseStateUpdate(
                Context::STATE_STRATEGY_BLOCKED);
    } catch (...) {
      AssertFailNoException();
    }
  }

  void RunAllAlgos() {
    // Not supported as was not required before:
    Assert(!ThreadPositionListTransaction::IsStarted());

    const auto &end = m_positions.GetEnd();
    for (auto it = m_positions.GetBegin(); it != end; ++it) {
      it->RunAlgos();
    }
  }

  void RaiseSinglePositionUpdateEvent(Position &position) {
    // Not supported as was not required before:
    Assert(!ThreadPositionListTransaction::IsStarted());

    Assert(m_positions.Has(position));

    if (position.IsError()) {
      m_strategy.GetLog().Error("Will be blocked by position error...");
      m_strategy.Block();
      ForgetPosition(position);
      //! @todo notify engine here
      return;
    } else if (position.IsInactive()) {
      const auto &blockPeriod = pt::seconds(1);
      m_strategy.GetLog().Error(
          "Will be blocked by position inactivity at %1%...", blockPeriod);
      m_strategy.Block(blockPeriod);
    }

    const bool isCompleted = position.IsCompleted();
    try {
      Assert(!position.IsCancelling());
      position.RunAlgos();
      if (!position.IsCancelling()) {
        m_strategy.OnPositionUpdate(position);
      }
    } catch (const ::trdk::Lib::RiskControlException &ex) {
      BlockByRiskControlEvent(ex, "position update");
      return;
    }

    if (isCompleted) {
      ForgetPosition(position);
    }
  }

  void FlushDelayed(Lock &lock) {
    // Not supported as was not required before:
    Assert(!ThreadPositionListTransaction::IsStarted());

    while (!m_delayedPositionToForget.empty()) {
      lock.unlock();
      boost::this_thread::yield();
      lock.lock();
      if (m_delayedPositionToForget.empty()) {
        break;
      }
      Assert(m_delayedPositionToForget.back());
      auto &delayedPosition = *m_delayedPositionToForget.back();
      m_delayedPositionToForget.pop_back();
      Assert(delayedPosition.IsCompleted());
      // It may also be in the ThreadPositionListTransaction::GetInstance by
      // this case was not required before as no one calls "make completed"
      // from async task:
      Assert(m_positions.Has(delayedPosition));
      RaiseSinglePositionUpdateEvent(delayedPosition);
      Assert(!m_positions.Has(delayedPosition) || m_strategy.IsBlocked());
    }
  }
};

boost::thread_specific_ptr<
    Strategy::Implementation::ThreadPositionListTransaction::Data>
    Strategy::Implementation::ThreadPositionListTransaction::m_instance;

//////////////////////////////////////////////////////////////////////////

Strategy::Strategy(trdk::Context &context,
                   const std::string &typeUuid,
                   const std::string &implementationName,
                   const std::string &instanceName,
                   const IniSectionRef &conf)
    : Consumer(context, "Strategy", implementationName, instanceName, conf),
      m_pimpl(boost::make_unique<Implementation>(*this, typeUuid, conf)) {
  std::string dropCopyInstanceIdStr = "not used";
  GetContext().InvokeDropCopy([this,
                               &dropCopyInstanceIdStr](DropCopy &dropCopy) {
    m_pimpl->m_dropCopyInstanceId = dropCopy.RegisterStrategyInstance(*this);
    AssertNe(DropCopy::nStrategyInstanceId, m_pimpl->m_dropCopyInstanceId);
    dropCopyInstanceIdStr =
        boost::lexical_cast<std::string>(m_pimpl->m_dropCopyInstanceId);
  });

  GetLog().Info("%1%, %2% mode, drop copy ID: %3%.",
                m_pimpl->m_isEnabled ? "ENABLED" : "DISABLED",
                boost::to_upper_copy(ConvertToString(GetTradingMode())),
                dropCopyInstanceIdStr);
}

Strategy::~Strategy() {
  Assert(!Implementation::ThreadPositionListTransaction::IsStarted());
  try {
    if (!m_pimpl->m_positions.IsEmpty()) {
      GetLog().Info("%1% active position(s).", m_pimpl->m_positions.GetSize());
    }
  } catch (...) {
    AssertFailNoException();
  }
}

const uuids::uuid &Strategy::GetTypeId() const { return m_pimpl->m_typeId; }

const std::string &Strategy::GetTitle() const { return m_pimpl->m_title; }

TradingMode Strategy::GetTradingMode() const { return m_pimpl->m_tradingMode; }

const DropCopyStrategyInstanceId &Strategy::GetDropCopyInstanceId() const {
  AssertNe(DropCopy::nStrategyInstanceId, m_pimpl->m_dropCopyInstanceId);
  return m_pimpl->m_dropCopyInstanceId;
}

RiskControlScope &Strategy::GetRiskControlScope() {
  return *m_pimpl->m_riskControlScope;
}

TradingSystem &Strategy::GetTradingSystem(size_t index) {
  return GetContext().GetTradingSystem(index, GetTradingMode());
}
const TradingSystem &Strategy::GetTradingSystem(size_t index) const {
  return GetContext().GetTradingSystem(index, GetTradingMode());
}

void Strategy::OnLevel1Update(Security &security,
                              const TimeMeasurement::Milestones &) {
  GetLog().Error(
      "Subscribed to %1% level 1 updates, but can't work with it"
      " (doesn't have OnLevel1Update method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to level 1 updates, but can't work with it");
}

void Strategy::OnPositionUpdate(Position &) {}

void Strategy::OnBookUpdateTick(Security &security,
                                const PriceBook &,
                                const TimeMeasurement::Milestones &) {
  GetLog().Error(
      "Subscribed to %1% book Update ticks, but can't work with it"
      " (doesn't have OnBookUpdateTick method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to book Update ticks, but can't work with it");
}

void Strategy::Register(Position &position) {
  PositionHolder holder(position, position.Subscribe([this, &position]() {
    m_pimpl->m_positionUpdateSignal(position);
  }));
  Implementation::ThreadPositionListTransaction::IsStarted()
      ? Implementation::ThreadPositionListTransaction::GetData().Insert(
            std::move(holder))
      : m_pimpl->m_positions.Insert(std::move(holder));
}

void Strategy::Unregister(Position &position) noexcept {
  try {
    Implementation::ThreadPositionListTransaction::IsStarted()
        ? Implementation::ThreadPositionListTransaction::GetData().Erase(
              position)
        : m_pimpl->m_positions.Erase(position);
  } catch (...) {
    AssertFailNoException();
    Block();
  }
}

void Strategy::RaiseLevel1UpdateEvent(
    Security &security, const TimeMeasurement::Milestones &delayMeasurement) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  delayMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
  try {
    m_pimpl->RunAllAlgos();
    OnLevel1Update(security, delayMeasurement);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "level 1 update");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseLevel1TickEvent(
    trdk::Security &security,
    const boost::posix_time::ptime &time,
    const Level1TickValue &value,
    const TimeMeasurement::Milestones &delayMeasurement) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  delayMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
  try {
    m_pimpl->RunAllAlgos();
    OnLevel1Tick(security, time, value, delayMeasurement);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "level 1 tick");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseNewTradeEvent(Security &service,
                                  const boost::posix_time::ptime &time,
                                  const Price &price,
                                  const Qty &qty) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnNewTrade(service, time, price, qty);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "new trade");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseServiceDataUpdateEvent(
    const Service &service,
    const TimeMeasurement::Milestones &timeMeasurement) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnServiceDataUpdate(service, timeMeasurement);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "service data update");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaisePositionUpdateEvent(Position &position) {
  Assert(position.IsStarted());
  auto lock = LockForOtherThreads();
  if (position.IsCompleted() && !m_pimpl->m_positions.Has(position)) {
    return;
  }
  m_pimpl->RaiseSinglePositionUpdateEvent(position);
  m_pimpl->FlushDelayed(lock);
}

void Strategy::OnPositionMarkedAsCompleted(Position &position) {
  //! @todo Extend: delay several positions, don't remove from callback,
  //! don't forget about all other callbacks where positions can be used.
  Assert(position.IsCompleted());
  Assert(std::find(m_pimpl->m_delayedPositionToForget.cbegin(),
                   m_pimpl->m_delayedPositionToForget.cend(),
                   &position) == m_pimpl->m_delayedPositionToForget.cend());
  m_pimpl->m_delayedPositionToForget.emplace_back(&position);
}

void Strategy::RaiseSecurityContractSwitchedEvent(const pt::ptime &time,
                                                  Security &security,
                                                  Security::Request &request,
                                                  bool &isSwitched) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnSecurityContractSwitched(time, security, request, isSwitched);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "security contract switched");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseBrokerPositionUpdateEvent(Security &security,
                                              bool isLong,
                                              const Qty &qty,
                                              const Volume &volume,
                                              bool isInitial) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnBrokerPositionUpdate(security, isLong, qty, volume, isInitial);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "broker position update");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseNewBarEvent(Security &security, const Security::Bar &bar) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnNewBar(security, bar);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "new bar");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseBookUpdateTickEvent(
    Security &security,
    const PriceBook &book,
    const TimeMeasurement::Milestones &timeMeasurement) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
  try {
    OnBookUpdateTick(security, book, timeMeasurement);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "book update tick");
  }
  m_pimpl->FlushDelayed(lock);
}

void Strategy::RaiseSecurityServiceEvent(const pt::ptime &time,
                                         Security &security,
                                         const Security::ServiceEvent &event) {
  auto lock = LockForOtherThreads();
  // 1st time already checked: before enqueue event (without locking),
  // here - control check (under mutex as blocking and enabling - under
  // the mutex too):
  if (IsBlocked()) {
    return;
  }
  try {
    OnSecurityServiceEvent(time, security, event);
  } catch (const ::trdk::Lib::RiskControlException &ex) {
    m_pimpl->BlockByRiskControlEvent(ex, "security service event");
  }
  m_pimpl->FlushDelayed(lock);
}

bool Strategy::IsBlocked(bool isForever) const {
  if (!m_pimpl->m_isEnabled) {
    return true;
  }

  if (!m_pimpl->m_isBlocked) {
    return false;
  }

  const Implementation::BlockLock lock(m_pimpl->m_blockMutex);

  if (m_pimpl->m_blockEndTime == pt::not_a_date_time ||
      m_pimpl->m_blockEndTime > GetContext().GetCurrentTime()) {
    return true;
  } else if (isForever) {
    return false;
  }

  m_pimpl->m_blockEndTime = pt::not_a_date_time;
  m_pimpl->m_isBlocked = false;

  GetLog().Info("Unblocked.");

  return false;
}

void Strategy::Block() noexcept { m_pimpl->Block(); }

void Strategy::Block(const std::string &reason) noexcept {
  m_pimpl->Block(&reason);
}

void Strategy::Block(const pt::time_duration &blockDuration) {
  const Implementation::BlockLock lock(m_pimpl->m_blockMutex);
  const pt::ptime &blockEndTime = GetContext().GetCurrentTime() + blockDuration;
  if (m_pimpl->m_isBlocked && m_pimpl->m_blockEndTime != pt::not_a_date_time &&
      blockEndTime <= m_pimpl->m_blockEndTime) {
    return;
  }
  m_pimpl->m_isBlocked = true;
  m_pimpl->m_blockEndTime = blockEndTime;
  GetLog().Warn("Blocked until %1%.", m_pimpl->m_blockEndTime);
}

void Strategy::Stop(const StopMode &stopMode) {
  const auto lock = LockForOtherThreads();
  m_pimpl->m_stopMode = stopMode;
  OnStopRequest(stopMode);
}

void Strategy::OnStopRequest(const StopMode &) { ReportStop(); }

void Strategy::ReportStop() {
  const Implementation::BlockLock lock(m_pimpl->m_blockMutex);

  static_assert(numberOfStopModes == 3, "Stop mode list changed.");
  switch (GetStopMode()) {
    case STOP_MODE_GRACEFULLY_ORDERS:
      for (const auto &pos : GetPositions()) {
        if (pos.HasActiveOrders()) {
          GetLog().Error(
              "Found position %1%/%2% with active orders at stop"
              " with mode \"wait for orders before\".",
              pos.GetOperation()->GetId(),  // 1
              pos.GetSubOperationId());     // 2
        }
        Assert(!pos.HasActiveOrders());
      }
      break;
    case STOP_MODE_GRACEFULLY_POSITIONS:
      if (!GetPositions().IsEmpty()) {
        GetLog().Error(
            "Found %1% active positions at stop"
            " with mode \"wait for positions before\".",
            GetPositions().GetSize());
        Assert(GetPositions().IsEmpty());
      }
      break;
    case STOP_MODE_UNKNOWN:
      throw LogicError("Strategy stop not requested");
      break;
  }

  m_pimpl->m_isBlocked = true;
  m_pimpl->m_blockEndTime = pt::not_a_date_time;

  GetLog().Info("Stopped.");
  m_pimpl->m_stopCondition.notify_all();
}

const StopMode &Strategy::GetStopMode() const { return m_pimpl->m_stopMode; }

void Strategy::WaitForStop() {
  Implementation::BlockLock lock(m_pimpl->m_blockMutex);
  if (m_pimpl->m_isBlocked && m_pimpl->m_blockEndTime == pt::not_a_date_time) {
    return;
  }
  m_pimpl->m_stopCondition.wait(lock);
  Assert(m_pimpl->m_isBlocked);
  AssertEq(m_pimpl->m_blockEndTime, pt::not_a_date_time);
}

Strategy::PositionUpdateSlotConnection Strategy::SubscribeToPositionsUpdates(
    const PositionUpdateSlot &slot) const {
  return m_pimpl->m_positionUpdateSignal.connect(slot);
}

Strategy::PositionList &Strategy::GetPositions() {
  // Not supported as was not required before:
  Assert(!Implementation::ThreadPositionListTransaction::IsStarted());
  return m_pimpl->m_positions;
}

const Strategy::PositionList &Strategy::GetPositions() const {
  return const_cast<Strategy *>(this)->GetPositions();
}

std::unique_ptr<Strategy::PositionListTransaction>
Strategy::StartThreadPositionsTransaction() {
  return boost::make_unique<Implementation::ThreadPositionListTransaction>(
      m_pimpl->m_positions);
}

void Strategy::OnSettingsUpdate(const IniSectionRef &conf) {
  Consumer::OnSettingsUpdate(conf);

  if (m_pimpl->m_isEnabled != conf.ReadBoolKey("is_enabled")) {
    m_pimpl->m_isEnabled = !m_pimpl->m_isEnabled;
    GetLog().Info("%1%.", m_pimpl->m_isEnabled ? "ENABLED" : "DISABLED");
  }

  m_pimpl->m_riskControlScope->OnSettingsUpdate(conf);
}

void Strategy::ClosePositions() {
  const auto lock = LockForOtherThreads();
  GetLog().Info("Closing positions by request...");
  OnPostionsCloseRequest();
}

////////////////////////////////////////////////////////////////////////////////
