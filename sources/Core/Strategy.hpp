/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Consumer.hpp"
#include "Position.hpp"

namespace trdk {

class TRDK_CORE_API Strategy : public trdk::Consumer {
 public:
  typedef void(PositionUpdateSlotSignature)(trdk::Position &);
  typedef boost::function<PositionUpdateSlotSignature> PositionUpdateSlot;
  typedef boost::signals2::connection PositionUpdateSlotConnection;

  class TRDK_CORE_API PositionList {
   public:
    class ConstIterator;

    class TRDK_CORE_API Iterator
        : public boost::iterator_facade<Iterator,
                                        trdk::Position,
                                        boost::bidirectional_traversal_tag> {
      friend class trdk::Strategy::PositionList::ConstIterator;

     public:
      class Implementation;

     public:
      explicit Iterator(Implementation *) noexcept;
      Iterator(const Iterator &);
      ~Iterator();

     public:
      Iterator &operator=(const Iterator &);
      void Swap(Iterator &);

     public:
      trdk::Position &dereference() const;
      bool equal(const Iterator &) const;
      bool equal(const ConstIterator &) const;
      void increment();
      void decrement();
      void advance(const difference_type &);

     private:
      Implementation *m_pimpl;
    };

    class TRDK_CORE_API ConstIterator
        : public boost::iterator_facade<ConstIterator,
                                        const trdk::Position,
                                        boost::bidirectional_traversal_tag> {
      friend class trdk::Strategy::PositionList::Iterator;

     public:
      class Implementation;

     public:
      explicit ConstIterator(Implementation *) noexcept;
      explicit ConstIterator(const Iterator &) noexcept;
      ConstIterator(const ConstIterator &);
      ~ConstIterator();

     public:
      ConstIterator &operator=(const ConstIterator &);
      void Swap(ConstIterator &);

     public:
      const trdk::Position &dereference() const;
      bool equal(const Iterator &) const;
      bool equal(const ConstIterator &) const;
      void increment();
      void decrement();
      void advance(const difference_type &);

     private:
      class Implementation;
      std::unique_ptr<Implementation> m_pimpl;
    };

   public:
    virtual ~PositionList() = default;

   public:
    Iterator begin() { return GetBegin(); }
    ConstIterator cbegin() const { return GetBegin(); }
    Iterator end() { return GetEnd(); }
    ConstIterator cend() const { return GetEnd(); }

   public:
    virtual size_t GetSize() const = 0;
    virtual bool IsEmpty() const = 0;

    virtual Iterator GetBegin() = 0;
    virtual ConstIterator GetBegin() const = 0;

    virtual Iterator GetEnd() = 0;
    virtual ConstIterator GetEnd() const = 0;
  };

  class PositionListTransaction : private boost::noncopyable {
   public:
    virtual ~PositionListTransaction() = default;
  };

 public:
  explicit Strategy(trdk::Context &,
                    const std::string &typeUuid,
                    const std::string &implementationName,
                    const std::string &instanceName,
                    const trdk::Lib::IniSectionRef &);
  virtual ~Strategy() override;

 public:
  const boost::uuids::uuid &GetTypeId() const;
  const std::string &GetTitle() const;
  trdk::TradingMode GetTradingMode() const;
  const trdk::DropCopyStrategyInstanceId &GetDropCopyInstanceId() const;

 public:
  trdk::RiskControlScope &GetRiskControlScope();

  trdk::TradingSystem &GetTradingSystem(size_t index);
  const trdk::TradingSystem &GetTradingSystem(size_t index) const;

 public:
  bool IsBlocked(bool isForever = false) const;
  void WaitForStop();

  void Block() noexcept;
  void Block(const std::string &reason) noexcept;
  void Block(const boost::posix_time::time_duration &);
  void Stop(const trdk::StopMode &);

  void ClosePositions();

  //! Safely invokes an action with the instance of the strategy.
  /** Applied only for non-system calls.
    */
  template <typename StrategyImplementation, typename Callback>
  void Invoke(const Callback &callback) {
    const auto lock = LockForOtherThreads();
    if (IsBlocked()) {
      throw trdk::Lib::Exception("Strategy is blocked");
    }
    StrategyImplementation *const impl =
        dynamic_cast<StrategyImplementation *>(this);
    if (!impl) {
      throw trdk::Lib::Exception(
          "Strategy requested to invoke has another type");
    }
    callback(*impl);
  }
  /** Applied only for non-system calls.
    */
  template <typename StrategyImplementation, typename Callback>
  void Invoke(const Callback &callback) const {
    const auto &lock = LockForOtherThreads();
    if (IsBlocked()) {
      throw trdk::Lib::Exception("Strategy is blocked");
    }
    const StrategyImplementation *const impl =
        dynamic_cast<const StrategyImplementation *>(this);
    if (!impl) {
      throw trdk::Lib::Exception(
          "Strategy requested to invoke has another type");
    }
    callback(*impl);
  }

 public:
  virtual void RaiseSecurityContractSwitchedEvent(
      const boost::posix_time::ptime &,
      Security &,
      Security::Request &,
      bool &isSwitched) override;

  virtual void RaiseBrokerPositionUpdateEvent(trdk::Security &,
                                              bool isLong,
                                              const trdk::Qty &,
                                              const trdk::Volume &,
                                              bool isInitial) override;

  virtual void RaiseNewBarEvent(trdk::Security &, const trdk::Security::Bar &);

  virtual void RaiseSecurityServiceEvent(
      const boost::posix_time::ptime &,
      trdk::Security &,
      const trdk::Security::ServiceEvent &) override;

  void RaiseLevel1UpdateEvent(trdk::Security &,
                              const trdk::Lib::TimeMeasurement::Milestones &);
  void RaiseLevel1TickEvent(trdk::Security &,
                            const boost::posix_time::ptime &,
                            const trdk::Level1TickValue &,
                            const trdk::Lib::TimeMeasurement::Milestones &);
  void RaiseNewTradeEvent(trdk::Security &,
                          const boost::posix_time::ptime &,
                          const trdk::Price &,
                          const trdk::Qty &);
  void RaiseServiceDataUpdateEvent(
      const trdk::Service &, const trdk::Lib::TimeMeasurement::Milestones &);
  void RaisePositionUpdateEvent(trdk::Position &);
  void RaiseBookUpdateTickEvent(trdk::Security &,
                                const trdk::PriceBook &,
                                const trdk::Lib::TimeMeasurement::Milestones &);

 public:
  //! Registers position for this strategy.
  /** Thread-unsafe method! Must be called only from event-methods, or if
    * strategy locked by GetMutex().
    */
  virtual void Register(Position &);
  //! Unregisters position for this strategy.
  /** Thread-unsafe method! Must be called only from event-methods, or if
    * strategy locked by GetMutex().
    */
  virtual void Unregister(Position &) noexcept;

  PositionList &GetPositions();
  const PositionList &GetPositions() const;

  std::unique_ptr<PositionListTransaction> StartThreadPositionsTransaction();

 public:
  PositionUpdateSlotConnection SubscribeToPositionsUpdates(
      const PositionUpdateSlot &) const;

  void OnPositionMarkedAsCompleted(Position &);

 protected:
  virtual void OnLevel1Update(trdk::Security &,
                              const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void OnPositionUpdate(trdk::Position &);
  virtual void OnBookUpdateTick(trdk::Security &,
                                const trdk::PriceBook &,
                                const trdk::Lib::TimeMeasurement::Milestones &);
  virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

  const trdk::StopMode &GetStopMode() const;
  virtual void OnStopRequest(const trdk::StopMode &);
  void ReportStop();

  virtual void OnPostionsCloseRequest() = 0;

  //! Will be called after position blocking.
  /** @param[in] Blocking reason, if existent (maybe nullptr).
    * @return True, if position blocking event should be broadcasted through the
    *         engine subscription, false otherwise (position still be blocked
    *         in any case).
    */
  virtual bool OnBlocked(const std::string *reason = nullptr) noexcept;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}

//////////////////////////////////////////////////////////////////////////

namespace trdk {

inline trdk::Strategy::PositionList::Iterator range_begin(
    trdk::Strategy::PositionList &list) {
  return list.GetBegin();
}
inline trdk::Strategy::PositionList::Iterator range_end(
    trdk::Strategy::PositionList &list) {
  return list.GetEnd();
}
inline trdk::Strategy::PositionList::ConstIterator range_begin(
    const trdk::Strategy::PositionList &list) {
  return list.GetBegin();
}
inline trdk::Strategy::PositionList::ConstIterator range_end(
    const trdk::Strategy::PositionList &list) {
  return list.GetEnd();
}
}

namespace boost {
template <>
struct range_mutable_iterator<trdk::Strategy::PositionList> {
  typedef trdk::Strategy::PositionList::Iterator type;
};
template <>
struct range_const_iterator<trdk::Strategy::PositionList> {
  typedef trdk::Strategy::PositionList::ConstIterator type;
};
}

namespace std {
template <>
inline void swap(trdk::Strategy::PositionList::Iterator &lhs,
                 trdk::Strategy::PositionList::Iterator &rhs) {
  lhs.Swap(rhs);
}
template <>
inline void swap(trdk::Strategy::PositionList::ConstIterator &lhs,
                 trdk::Strategy::PositionList::ConstIterator &rhs) {
  lhs.Swap(rhs);
}
}

//////////////////////////////////////////////////////////////////////////
