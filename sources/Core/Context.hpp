/**************************************************************************
 *   Created: 2013/01/31 01:04:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "EventsLog.hpp"

namespace trdk {

class TRDK_CORE_API Context {
 public:
  class TRDK_CORE_API Exception : public Lib::Exception {
   public:
    explicit Exception(const char* what) noexcept;
  };

  class TRDK_CORE_API TradingModeIsNotLoaded : public Exception {
   public:
    explicit TradingModeIsNotLoaded(const char* what) noexcept;
  };

  typedef EventsLog Log;
  typedef TradingLog TradingLog;

  class TRDK_CORE_API DispatchingLock {
   public:
    DispatchingLock() = default;
    DispatchingLock(DispatchingLock&&) = default;
    DispatchingLock(const DispatchingLock&) = delete;
    DispatchingLock& operator=(DispatchingLock&&) = delete;
    DispatchingLock& operator=(const DispatchingLock&) = delete;
    virtual ~DispatchingLock() = 0;
  };

  typedef void(CurrentTimeChangeSlotSignature)(
      const boost::posix_time::ptime& newTime);
  typedef boost::function<CurrentTimeChangeSlotSignature> CurrentTimeChangeSlot;
  typedef boost::signals2::connection CurrentTimeChangeSlotConnection;

  enum State {
    STATE_ENGINE_STARTED,
    STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY,
    STATE_DISPATCHER_TASK_STOPPED_ERROR,
    STATE_STRATEGY_BLOCKED,
    numberOfStates,
  };

  typedef void(StateUpdateSlotSignature)(
      const State& newState, const std::string* message /*= nullptr*/);
  typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
  typedef boost::signals2::connection StateUpdateConnection;

  explicit Context(Log&, TradingLog&, Settings&&);
  Context(Context&&);
  Context(const Context&) = delete;
  Context& operator=(Context&&) = delete;
  Context& operator=(const Context&) = delete;
  virtual ~Context();

  Log& GetLog() const noexcept;
  TradingLog& GetTradingLog() const noexcept;

  const Timer& GetTimer();

  //! Subscribes to state changes.
  StateUpdateConnection SubscribeToStateUpdates(const StateUpdateSlot&) const;
  //! Raises state update event.
  /** Engine service should listen events and react to it.
   */
  void RaiseStateUpdate(const State&) const;
  //! Raises state update event with message.
  /** Engine service should listen events and react to it.
   */
  void RaiseStateUpdate(const State&, const std::string& message) const;

  Lib::TimeMeasurement::Milestones StartStrategyTimeMeasurement() const;
  Lib::TimeMeasurement::Milestones StartTradingSystemTimeMeasurement() const;
  Lib::TimeMeasurement::Milestones StartDispatchingTimeMeasurement() const;

  //! Context setting with predefined key list and predefined behavior.
  const Settings& GetSettings() const;

  //! Context local start time.
  /**
   * Does not depend from replay mode, always actual start time for local time
   * zone.
   * @sa GetCurrentTime
   */
  const boost::posix_time::ptime& GetStartTime() const;

  //! Current time.
  /**
   * Initialization, value and time zone depends from settings and replay mode.
   * @sa GetStartTime
   */
  virtual boost::posix_time::ptime GetCurrentTime() const;
  //! The current time in the specific timezone.
  /**
   * Initialization, value and timezone depends from settings and replay mode.
   * @sa GetStartTime
   */
  boost::posix_time::ptime GetCurrentTime(
      const boost::local_time::time_zone_ptr&) const;

  //! Sets current time (for replay mode).
  /**
   * @param newTime New time, can be only greater or equal then current.
   * @param signalAboutUpdate If true - signal about update will be sent for all
   *                          subscribers.
   * @sa SubscribeToCurrentTimeChange
   */
  void SetCurrentTime(const boost::posix_time::ptime& newTime,
                      bool signalAboutUpdate);
  //! Subscribes to a time change by SetCurrentTime.
  /**
   * Signals before new time will be set for context.
   * @sa SetCurrentTime
   */
  CurrentTimeChangeSlotConnection SubscribeToCurrentTimeChange(
      const CurrentTimeChangeSlot&) const;

  //! Waits until each of dispatching queue will be empty (but not all
  //! at the same moment).
  virtual std::unique_ptr<DispatchingLock> SyncDispatching() const = 0;

  virtual RiskControl& GetRiskControl(const TradingMode&) = 0;
  virtual const RiskControl& GetRiskControl(const TradingMode&) const = 0;

  template <typename Method>
  void InvokeDropCopy(const Method& method) const {
    auto* const dropCopy = GetDropCopy();
    if (!dropCopy) {
      return;
    }
    try {
      method(*dropCopy);
    } catch (const std::exception& ex) {
      GetLog().Error("Failed to invoke drop copy: \"%1%\".", ex.what());
    }
  }

  //! Returns expiration calendar.
  /**
   * Throws an exception if expiration calendar is not set.
   * @sa HasExpirationCalendar
   * @throw trdk::Lib::Exception
   */
  virtual const Lib::ExpirationCalendar& GetExpirationCalendar() const = 0;
  //! Returns true if expiration calendar is set.
  /** @sa GetExpirationCalendar
   */
  virtual bool HasExpirationCalendar() const = 0;

  //! Market Data Sources count.
  /** @sa GetMarketDataSource
   */
  virtual size_t GetNumberOfMarketDataSources() const = 0;
  //! Returns Market Data Source by index.
  /**
   * Throws an exception if index in unknown.
   * @sa GetNumberOfMarketDataSources
   * @throw trdk::Lib::Exception
   */
  virtual const MarketDataSource& GetMarketDataSource(size_t index) const = 0;
  //! Returns Market Data Source by index.
  /**
   * Throws an exception if index in unknown.
   * @sa GetNumberOfMarketDataSources
   * @throw trdk::Lib::Exception
   */
  virtual MarketDataSource& GetMarketDataSource(size_t index) = 0;
  //! Applies the given predicate to the each market data source.
  virtual void ForEachMarketDataSource(
      const boost::function<void(const MarketDataSource&)>&) const = 0;
  //! Applies the given callback to the each market data source.
  virtual void ForEachMarketDataSource(
      const boost::function<void(MarketDataSource&)>&) = 0;

  //! Trading Systems count.
  /** @sa GetTradingSystem
   */
  virtual size_t GetNumberOfTradingSystems() const = 0;
  //! Returns Trading System by index.
  /**
   * Throws an exception if index in unknown.
   * @sa GetNumberOfTradingSystems
   * @throw trdk::Lib::Exception
   */
  virtual const TradingSystem& GetTradingSystem(size_t index,
                                                const TradingMode&) const = 0;
  //! Returns Trading System by index.
  /**
   * Throws an exception if index in unknown.
   * @sa GetNumberOfTradingSystems
   * @throw trdk::Lib::Exception
   */
  virtual TradingSystem& GetTradingSystem(size_t index, const TradingMode&) = 0;

  virtual Strategy& GetSrategy(const boost::uuids::uuid& id) = 0;
  virtual const Strategy& GetSrategy(const boost::uuids::uuid& id) const = 0;

  //! Asks each strategy to close all opened positions if it has.
  virtual void CloseSrategiesPositions() = 0;

  virtual void Add(const boost::property_tree::ptree&) = 0;

 protected:
  //! Returns Drop Copy or nullptr.
  virtual DropCopy* GetDropCopy() const = 0;

  void OnStarted();
  void OnBeforeStop();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace trdk
