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

class TRDK_CORE_API Strategy : public Consumer {
 public:
  typedef void(PositionUpdateSlotSignature)(Position &);
  typedef boost::function<PositionUpdateSlotSignature> PositionUpdateSlot;
  typedef boost::signals2::connection PositionUpdateSlotConnection;

  class TRDK_CORE_API PositionList {
   public:
    class ConstIterator;

    class TRDK_CORE_API Iterator
        : public boost::iterator_facade<Iterator,
                                        Position,
                                        boost::bidirectional_traversal_tag> {
      friend class ConstIterator;

     public:
      class Implementation;

      explicit Iterator(Implementation *) noexcept;
      Iterator(const Iterator &);
      ~Iterator();

      Iterator &operator=(const Iterator &);
      void Swap(Iterator &) noexcept;

      Position &dereference() const;
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
                                        const Position,
                                        boost::bidirectional_traversal_tag> {
      friend class Iterator;

     public:
      class Implementation;

      explicit ConstIterator(Implementation *) noexcept;
      explicit ConstIterator(const Iterator &) noexcept;
      ConstIterator(const ConstIterator &);
      ~ConstIterator();

      ConstIterator &operator=(const ConstIterator &);
      void Swap(ConstIterator &);

      const Position &dereference() const;
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
    class Data : private boost::noncopyable {
     public:
      virtual ~Data() = default;
    };

   public:
    virtual ~PositionListTransaction() = default;

   public:
    virtual std::unique_ptr<Data> MoveToThread() = 0;
  };

 public:
  explicit Strategy(Context &,
                    const boost::uuids::uuid &typeUuid,
                    const std::string &implementationName,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &);
  explicit Strategy(Context &,
                    const std::string &typeUuid,
                    const std::string &implementationName,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &);
  ~Strategy() override;

  const boost::uuids::uuid &GetTypeId() const;
  const std::string &GetTitle() const;
  TradingMode GetTradingMode() const;
  const DropCopyStrategyInstanceId &GetDropCopyInstanceId() const;

  RiskControlScope &GetRiskControlScope();

  TradingSystem &GetTradingSystem(size_t index);
  const TradingSystem &GetTradingSystem(size_t index) const;

 public:
  bool IsBlocked(bool isForever = false) const;
  void WaitForStop();

  void Block() noexcept;
  void Block(const std::string &reason) noexcept;
  void Block(const boost::posix_time::time_duration &);
  void Stop(const StopMode &);

  void ClosePositions();

  //! Schedules call after time. Callback have to be available before will ba
  //! called or while module will not be destroyed.
  void Schedule(const boost::posix_time::time_duration &,
                boost::function<void()> &&);
  //! Schedules call immediately. Callback have to be available before will ba
  //! called or while module will not be destroyed.
  void Schedule(boost::function<void()> &&);

 public:
  virtual void RaiseSecurityContractSwitchedEvent(
      const boost::posix_time::ptime &,
      Security &,
      Security::Request &,
      bool &isSwitched) override;

  virtual void RaiseBrokerPositionUpdateEvent(Security &,
                                              bool isLong,
                                              const Qty &,
                                              const Volume &,
                                              bool isInitial) override;

  virtual void RaiseNewBarEvent(Security &, const Security::Bar &);

  virtual void RaiseSecurityServiceEvent(
      const boost::posix_time::ptime &,
      Security &,
      const Security::ServiceEvent &) override;

  void RaiseLevel1UpdateEvent(Security &,
                              const Lib::TimeMeasurement::Milestones &);
  void RaiseLevel1TickEvent(Security &,
                            const boost::posix_time::ptime &,
                            const Level1TickValue &,
                            const Lib::TimeMeasurement::Milestones &);
  void RaiseNewTradeEvent(Security &,
                          const boost::posix_time::ptime &,
                          const Price &,
                          const Qty &);
  void RaiseServiceDataUpdateEvent(const Service &,
                                   const Lib::TimeMeasurement::Milestones &);
  void RaisePositionUpdateEvent(Position &);
  void RaiseBookUpdateTickEvent(Security &,
                                const PriceBook &,
                                const Lib::TimeMeasurement::Milestones &);

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
  virtual void OnLevel1Update(Security &,
                              const Lib::TimeMeasurement::Milestones &);
  virtual void OnPositionUpdate(Position &);
  virtual void OnBookUpdateTick(Security &,
                                const PriceBook &,
                                const Lib::TimeMeasurement::Milestones &);
  void OnSettingsUpdate(const Lib::IniSectionRef &) override;

  const StopMode &GetStopMode() const;
  virtual void OnStopRequest(const StopMode &);
  void ReportStop();

  virtual void OnPostionsCloseRequest() = 0;

  //! Will be called after position blocking.
  /** @param[in] reason Blocking reason, if existent (maybe nullptr).
   * @return True, if position blocking event should be broadcasted through the
   *         engine subscription, false otherwise (position still be blocked
   *         in any case).
   */
  virtual bool OnBlocked(const std::string *reason = nullptr) noexcept;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk

//////////////////////////////////////////////////////////////////////////

namespace trdk {

inline Strategy::PositionList::Iterator range_begin(
    Strategy::PositionList &list) {
  return list.GetBegin();
}
inline Strategy::PositionList::Iterator range_end(
    Strategy::PositionList &list) {
  return list.GetEnd();
}
inline Strategy::PositionList::ConstIterator range_begin(
    const Strategy::PositionList &list) {
  return list.GetBegin();
}
inline Strategy::PositionList::ConstIterator range_end(
    const Strategy::PositionList &list) {
  return list.GetEnd();
}
}  // namespace trdk

namespace boost {
template <>
struct range_mutable_iterator<trdk::Strategy::PositionList> {
  typedef trdk::Strategy::PositionList::Iterator type;
};
template <>
struct range_const_iterator<trdk::Strategy::PositionList> {
  typedef trdk::Strategy::PositionList::ConstIterator type;
};
}  // namespace boost

namespace std {
template <>
inline void swap(trdk::Strategy::PositionList::Iterator &lhs,
                 trdk::Strategy::PositionList::Iterator &rhs) noexcept {
  lhs.Swap(rhs);
}
template <>
inline void swap(trdk::Strategy::PositionList::ConstIterator &lhs,
                 trdk::Strategy::PositionList::ConstIterator &rhs) noexcept {
  lhs.Swap(rhs);
}
}  // namespace std

//////////////////////////////////////////////////////////////////////////
