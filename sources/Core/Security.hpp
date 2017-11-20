/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Instrument.hpp"
#include "TradingSystem.hpp"

namespace trdk {

class TRDK_CORE_API Security : public trdk::Instrument {
 public:
  typedef trdk::Instrument Base;

  typedef uint32_t InstanceId;

  class TRDK_CORE_API Request {
   public:
    Request();
    operator bool() const;
    void Swap(Request &) throw();
    //! Returns true if one or more fields have earlier value.
    /** Does not cancel the reverse.
      */
    bool IsEarlier(const Request &) const;

   public:
    void RequestNumberOfTicks(size_t);
    void RequestTime(const boost::posix_time::ptime &);
    void Merge(const Request &);

   public:
    size_t GetNumberOfTicks() const;
    const boost::posix_time::ptime &GetTime() const;

   private:
    boost::posix_time::ptime m_time;
    size_t m_numberOfTicks;
  };

  typedef trdk::TradingSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

  typedef std::bitset<trdk::numberOfLevel1TickTypes> SupportedLevel1Types;

  typedef void(Level1UpdateSlotSignature)(
      const trdk::Lib::TimeMeasurement::Milestones &);
  //! Update one of more from following values:
  //! best bid, best ask, last trade.
  typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
  typedef boost::signals2::connection Level1UpdateSlotConnection;

  typedef void(Level1TickSlotSignature)(
      const boost::posix_time::ptime &,
      const trdk::Level1TickValue &,
      const trdk::Lib::TimeMeasurement::Milestones &,
      bool flush);
  typedef boost::function<Level1TickSlotSignature> Level1TickSlot;
  typedef boost::signals2::connection Level1TickSlotConnection;

  typedef void(NewTradeSlotSignature)(
      const boost::posix_time::ptime &,
      const trdk::Price &,
      const trdk::Qty &,
      const trdk::Lib::TimeMeasurement::Milestones &);
  typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
  typedef boost::signals2::connection NewTradeSlotConnection;

  //! Security broker position info.
  /** Information from broker, not relevant to trdk::Position.
    */
  typedef void(BrokerPositionUpdateSlotSignature)(bool isLong,
                                                  const trdk::Qty &,
                                                  const trdk::Volume &,
                                                  bool isInitial);
  typedef boost::function<BrokerPositionUpdateSlotSignature>
      BrokerPositionUpdateSlot;
  typedef boost::signals2::connection BrokerPositionUpdateSlotConnection;

  struct TRDK_CORE_API Bar {
    enum Type { TRADES, BID, ASK, numberOfTypes };

    //! Bar start time.
    boost::posix_time::ptime time;
    //! Data type.
    Type type;

    //! The bar opening price.
    boost::optional<trdk::Price> openPrice;
    //! The high price during the time covered by the bar.
    boost::optional<trdk::Price> highPrice;

    //! The low price during the time covered by the bar.
    boost::optional<trdk::Price> lowPrice;
    //! The bar closing price.
    boost::optional<trdk::Price> closePrice;

    //! The volume during the time covered by the bar.
    boost::optional<trdk::Qty> volume;

    //! Bar size (if bar-by-time). If period is not set - this is
    //! bar-by-points.
    /** @sa numberOfPoints
      */
    boost::optional<boost::posix_time::time_duration> period;
    //! Number of points (if existent).
    /** @sa period
      */
    boost::optional<size_t> numberOfPoints;

    explicit Bar(const boost::posix_time::ptime &, const Type &);
  };
  typedef void(NewBarSlotSignature)(const Bar &);
  typedef boost::function<NewBarSlotSignature> NewBarSlot;
  typedef boost::signals2::connection NewBarSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  typedef void(BookUpdateTickSlotSignature)(
      const trdk::PriceBook &, const trdk::Lib::TimeMeasurement::Milestones &);
  typedef boost::function<BookUpdateTickSlotSignature> BookUpdateTickSlot;
  typedef boost::signals2::connection BookUpdateTickSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  class TRDK_CORE_API Exception : public trdk::Lib::Exception {
   public:
    explicit Exception(const char *what);
  };

  class TRDK_CORE_API MarketDataValueDoesNotExist
      : public trdk::Security::Exception {
   public:
    explicit MarketDataValueDoesNotExist(const char *what);
  };

  ////////////////////////////////////////////////////////////////////////////////

  //! Security service event code.
  /** Notifies about system event for the specified security. Synchronous.
    *
    * @sa SubscribeToServiceEvents
    */
  enum ServiceEvent {
    //! History data is loaded, security is online now.
    /** @sa SERVICE_EVENT_OFFLINE
      */
    SERVICE_EVENT_ONLINE,
    //! Security is offline.
    /** @sa SERVICE_EVENT_ONLINE
      */
    SERVICE_EVENT_OFFLINE,
    //! The current trading session is opened.
    /** @sa SERVICE_EVENT_TRADING_SESSION_CLOSED
      */
    SERVICE_EVENT_TRADING_SESSION_OPENED,
    //! The current trading session is closed.
    /** There are no any special system limitation to trade or to get
      * information if the session is closed.
      * @sa SERVICE_EVENT_TRADING_SESSION_OPENED
      */
    SERVICE_EVENT_TRADING_SESSION_CLOSED,
    //! Number of events.
    numberOfServiceEvents
  };

  typedef void(ServiceEventSlotSignature)(const boost::posix_time::ptime &,
                                          const trdk::Security::ServiceEvent &);
  typedef boost::function<ServiceEventSlotSignature> ServiceEventSlot;
  typedef boost::signals2::connection ServiceEventSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  typedef void(ContractSwitchingSlotSignature)(const boost::posix_time::ptime &,
                                               const trdk::Security::Request &,
                                               bool &isSwitched);
  typedef boost::function<ContractSwitchingSlotSignature> ContractSwitchingSlot;
  typedef boost::signals2::connection ContractSwitchingSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

 public:
  explicit Security(trdk::Context &,
                    const trdk::Lib::Symbol &,
                    trdk::MarketDataSource &,
                    const SupportedLevel1Types &);
  virtual ~Security();

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &,
                                                const trdk::Security &);

 public:
  //! Returns Market Data Source object which provides market data for
  //! this security object.
  const trdk::MarketDataSource &GetSource() const;

  RiskControlSymbolContext &GetRiskControlContext(const trdk::TradingMode &);

 public:
  const trdk::Security::InstanceId &GetInstanceId() const;

  //! Returns true if security receives market data right now.
  /** @sa IsTradingSessionOpened
    */
  virtual bool IsOnline() const;

  //! Returns true if trading session is active at this moment.
  /** @sa IsOnline
    */
  virtual bool IsTradingSessionOpened() const;

  //! Sets requested data start time if it is not later than existing.
  void SetRequest(const trdk::Security::Request &);
  //! Returns requested data start.
  const trdk::Security::Request &GetRequest() const;

 public:
  size_t GetNumberOfItemsPerQty() const;
  const Qty &GetLotSize() const;

  uintmax_t GetPricePrecisionPower() const;
  uint8_t GetPricePrecision() const throw();

 public:
  boost::posix_time::ptime GetLastMarketDataTime() const;
  size_t TakeNumberOfMarketDataUpdates() const;

 public:
  virtual trdk::Price GetLastPrice() const;
  trdk::Qty GetLastQty() const;

  trdk::Price GetAskPrice() const;
  trdk::Price GetAskPriceValue() const;
  trdk::Qty GetAskQty() const;
  trdk::Qty GetAskQtyValue() const;

  trdk::Price GetBidPrice() const;
  trdk::Price GetBidPriceValue() const;
  trdk::Qty GetBidQty() const;
  trdk::Qty GetBidQtyValue() const;

  trdk::Qty GetTradedVolume() const;

  //! Returns next expiration time.
  /** Throws exception if expiration is not provided.
    * @sa HasExpiration
    */
  virtual const trdk::Lib::ContractExpiration &GetExpiration() const;
  //! Returns true if security has expiration.
  /** Throws exception if expiration is not provided.
    * @sa GetExpiration
    */
  bool HasExpiration() const;

 public:
  //! Subscribes to contract switching event.
  /** Notification should be synchronous. Notification generator will wait
    * until the event will be handled by each subscriber.
    */
  ContractSwitchingSlotConnection SubscribeToContractSwitching(
      const ContractSwitchingSlot &) const;

  Level1UpdateSlotConnection SubscribeToLevel1Updates(
      const Level1UpdateSlot &) const;

  Level1UpdateSlotConnection SubscribeToLevel1Ticks(
      const Level1TickSlot &) const;

  NewTradeSlotConnection SubscribeToTrades(const NewTradeSlot &) const;

  BrokerPositionUpdateSlotConnection SubscribeToBrokerPositionUpdates(
      const BrokerPositionUpdateSlot &) const;

  NewBarSlotConnection SubscribeToBars(const NewBarSlot &) const;

  BookUpdateTickSlotConnection SubscribeToBookUpdateTicks(
      const BookUpdateTickSlot &) const;

  //! Subscribes to security service event.
  /** Notification should be synchronous. Notification generator will wait
    * until the event will be handled by each subscriber.
    */
  ServiceEventSlotConnection SubscribeToServiceEvents(
      const ServiceEventSlot &) const;

 protected:
  //! Sets security online and generate event about it.
  /** @sa IsOnline
    * @return True, if state is changed, false - if state was the same before.
    */
  bool SetOnline(const boost::posix_time::ptime &, bool isOnline);
  void SetTradingSessionState(const boost::posix_time::ptime &, bool isOpened);
  void SwitchTradingSession(const boost::posix_time::ptime &);

  bool IsLevel1Required() const;
  bool IsLevel1UpdatesRequired() const;
  bool IsLevel1TicksRequired() const;
  bool IsTradesRequired() const;
  bool IsBrokerPositionRequired() const;
  bool IsBarsRequired() const;

  //! Sets one Level I parameter.
  /** Subscribers will be notified about Level I Update only if parameter
    * will bee changed.
    */
  void SetLevel1(const boost::posix_time::ptime &,
                 const trdk::Level1TickValue &,
                 const trdk::Lib::TimeMeasurement::Milestones &);
  //! Sets one Level I parameter.
  /** Subscribers will be notified about Level I Update only if parameter
    * will bee changed.
    * @return Returns true value was changed, false if set the same value as
    *         was.
    */
  bool SetLevel1(const boost::posix_time::ptime &,
                 const trdk::Level1TickValue &,
                 bool flush,
                 bool isPreviouslyChanged,
                 const trdk::Lib::TimeMeasurement::Milestones &);
  //! Sets two Level I parameters and one operation.
  /** More optimal than call "set one parameter" two times. Subscribers
    * will be notified about Level I Update only if parameter will be
    * changed.
    */
  void SetLevel1(const boost::posix_time::ptime &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Lib::TimeMeasurement::Milestones &);
  //! Sets three Level I parameters and one operation.
  /** More optimal than call "set one parameter" three times. Subscribers
    * will be notified about Level I Update only if parameter will bee
    * changed.
    */
  void SetLevel1(const boost::posix_time::ptime &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Lib::TimeMeasurement::Milestones &);
  //! Sets four Level I parameters and one operation.
  /** More optimal than call "set one parameter" four times. Subscribers
    * will be notified about Level I Update only if parameter will bee
    * changed.
    */
  void SetLevel1(const boost::posix_time::ptime &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Level1TickValue &,
                 const trdk::Lib::TimeMeasurement::Milestones &);
  //! Sets many Level I parameters and one operation.
  /** More optimal than call "set one parameter" several times.
    * Subscribers will be notified about Level I Update only if parameter will
    * bee changed.
    */
  void SetLevel1(const boost::posix_time::ptime &,
                 const std::vector<trdk::Level1TickValue> &,
                 const trdk::Lib::TimeMeasurement::Milestones &);

  //! Adds one Level I parameter tick.
  /** Subscribers will be notified about Level I Update only if parameter
    * will bee changed. Level I Tick event will be generated in any case.
    */
  bool AddLevel1Tick(const boost::posix_time::ptime &,
                     const trdk::Level1TickValue &,
                     bool flush,
                     bool isPreviouslyChanged,
                     const trdk::Lib::TimeMeasurement::Milestones &);

  //! Adds one Level I parameter tick.
  /** Subscribers will be notified about Level I Update only if parameter
    * will bee changed. Level I Tick event will be generated in any case.
    */
  void AddLevel1Tick(const boost::posix_time::ptime &,
                     const trdk::Level1TickValue &,
                     const trdk::Lib::TimeMeasurement::Milestones &);
  //! Adds two Level I parameter ticks.
  /** More optimal than call "add one tick" two times. Subscribers will
    * be notified about Level I Update only if parameter will be changed.
    * Level I Tick event will be generated in any case. First tick will be
    * added first in the notification queue, last - will be added last.
    * All ticks can have one type or various.
    */
  void AddLevel1Tick(const boost::posix_time::ptime &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Lib::TimeMeasurement::Milestones &);
  //! Adds three Level I parameter ticks.
  /** More optimal than call "add one tick" three times. Subscribers will
    * be notified about Level I Update only if parameter will be changed.
    * Level I Tick event will be generated in any case. First tick will be
    * added first in the notification queue, last - will be added last.
    * All ticks can have one type or various.
    */
  void AddLevel1Tick(const boost::posix_time::ptime &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Lib::TimeMeasurement::Milestones &);
  //! Adds four Level I parameter ticks.
  /** More optimal than call "add one tick" three times. Subscribers will
    * be notified about Level I Update only if parameter will be changed.
    * Level I Tick event will be generated in any case. First tick will be
    * added first in the notification queue, last - will be added last.
    * All ticks can have one type or various.
    */
  void AddLevel1Tick(const boost::posix_time::ptime &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Level1TickValue &,
                     const trdk::Lib::TimeMeasurement::Milestones &);
  //! Adds many Level I parameter ticks.
  /** More optimal than call "add one tick" several times. Subscribers
   ** will be notified about Level I Update only if parameter will be
   ** changed. Level I Tick event will be generated in any case. First
   ** tick will be added first in the notification queue, last - will be
   ** added last. All ticks can have one type or various.
    */
  void AddLevel1Tick(const boost::posix_time::ptime &,
                     const std::vector<trdk::Level1TickValue> &,
                     const trdk::Lib::TimeMeasurement::Milestones &);

  void AddTrade(const boost::posix_time::ptime &,
                const trdk::Price &,
                const trdk::Qty &,
                const trdk::Lib::TimeMeasurement::Milestones &,
                bool useAsLastTrade);

  void AddBar(Bar &&);

  //! Sets security broker position info.
  /** Subscribers will be notified only if parameter will be changed.
    * @param[in] isLong     If true - position has type "long", "short"
    *                       otherwise.
    * @param[in] qty        Position size.
    * @param[in] volume     Position volume.
    * @param[in] isInitial  true if it initial data at start.
    */
  void SetBrokerPosition(bool isLong,
                         const trdk::Qty &qty,
                         const Volume &volume,
                         bool isInitial);

  //! Sets price book snapshot with current data, but new time.
  /** Not thread-safe, caller should provide thread synchronization.
    * @param book New price book. Object can be used as temporary buffer and
    *             changed by this call, may have wrong data and state after.
    * @param timeMeasurement Time measurement object.
    */
  void SetBook(trdk::PriceBook &book,
               const trdk::Lib::TimeMeasurement::Milestones &timeMeasurement);

 public:
  //! Sets new expiration.
  /** All marked data for security will be reset (so if security just
    * started). The request will be reset. The request can be set new at
    * the event handling.
    *
    * @sa GetExpiration
    *
    * @param time[in] Event time.
    * @param expiration[in] New contract expiration.
    *
    * @return true if security has new request. Security will not have new
    *         request if no one subscriber request or if the new contract is
    *         first contract for security (if security did not have expiration
    *         before) and if new request after switching "is not earlier"
    *         the existing.
    */
  bool SetExpiration(const boost::posix_time::ptime &time,
                     const trdk::Lib::ContractExpiration &&expiration);
  //! Sets new expiration.
  /** All marked data for security will be reset (so if security just
    * started). The request will be reset. The request can be set new at
    * the event handling.
    *
    * @sa GetExpiration
    *
    * @param time[in] Event time.
    * @param expiration[in] New contract expiration.
    *
    * @return true if security has new request. Security will not have new
    *         request if no one subscriber request or if the new contract is
    *         first contract for security (if security did not have expiration
    *         before) and if new request after switching "is not earlier"
    *         the existing.
    */
  bool SetExpiration(const boost::posix_time::ptime &time,
                     const trdk::Lib::ContractExpiration &expiration);
  //! Continue contract switching which started by signal "contract switching
  //! signal".
  void ContinueContractSwitchingToNextExpiration();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
