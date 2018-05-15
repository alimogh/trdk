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

namespace trdk {

class TRDK_CORE_API Security : public Instrument {
 public:
  typedef Instrument Base;

  typedef uint32_t InstanceId;

  class TRDK_CORE_API Request {
   public:
    Request();
    explicit operator bool() const;
    void Swap(Request&) noexcept;
    //! Returns true if one or more fields have earlier value.
    /**
     * Does not cancel the reverse.
     */
    bool IsEarlier(const Request&) const;

    void RequestNumberOfTicks(size_t);
    void RequestTime(const boost::posix_time::ptime&);
    void Merge(const Request&);

    size_t GetNumberOfTicks() const;
    const boost::posix_time::ptime& GetTime() const;

   private:
    boost::posix_time::ptime m_time;
    size_t m_numberOfTicks;
  };

  typedef std::bitset<numberOfLevel1TickTypes> SupportedLevel1Types;

  typedef void(Level1UpdateSlotSignature)(
      const Lib::TimeMeasurement::Milestones&);
  //! Update one of more from following values: best bid, best ask, last trade.
  typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
  typedef boost::signals2::connection Level1UpdateSlotConnection;

  typedef void(Level1TickSlotSignature)(const boost::posix_time::ptime&,
                                        const Level1TickValue&,
                                        const Lib::TimeMeasurement::Milestones&,
                                        bool flush);
  typedef boost::function<Level1TickSlotSignature> Level1TickSlot;
  typedef boost::signals2::connection Level1TickSlotConnection;

  typedef void(NewTradeSlotSignature)(const boost::posix_time::ptime&,
                                      const Price&,
                                      const Qty&,
                                      const Lib::TimeMeasurement::Milestones&);
  typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
  typedef boost::signals2::connection NewTradeSlotConnection;

  //! Security broker position info.
  /** Information from broker, not relevant to trdk::Position.
   */
  typedef void(BrokerPositionUpdateSlotSignature)(bool isLong,
                                                  const Qty&,
                                                  const Volume&,
                                                  bool isInitial);
  typedef boost::function<BrokerPositionUpdateSlotSignature>
      BrokerPositionUpdateSlot;
  typedef boost::signals2::connection BrokerPositionUpdateSlotConnection;

  typedef void(NewBarSlotSignature)(const Bar&);
  typedef boost::function<NewBarSlotSignature> NewBarSlot;
  typedef boost::signals2::connection NewBarSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  typedef void(BookUpdateTickSlotSignature)(
      const PriceBook&, const Lib::TimeMeasurement::Milestones&);
  typedef boost::function<BookUpdateTickSlotSignature> BookUpdateTickSlot;
  typedef boost::signals2::connection BookUpdateTickSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  class TRDK_CORE_API Exception : public Lib::Exception {
   public:
    explicit Exception(const char* what);
  };

  class TRDK_CORE_API MarketDataValueDoesNotExist : public Exception {
   public:
    explicit MarketDataValueDoesNotExist(const char* what);
  };

  ////////////////////////////////////////////////////////////////////////////////

  //! Security service event code.
  /**
   * Notifies about system event for the specified security. Synchronous.
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
    /**
     * There are no any special system limitation to trade or to get information
     * if the session is closed.
     * @sa SERVICE_EVENT_TRADING_SESSION_OPENED
     */
    SERVICE_EVENT_TRADING_SESSION_CLOSED,
    //! Number of events.
    numberOfServiceEvents
  };

  typedef void(ServiceEventSlotSignature)(const boost::posix_time::ptime&,
                                          const ServiceEvent&);
  typedef boost::function<ServiceEventSlotSignature> ServiceEventSlot;
  typedef boost::signals2::connection ServiceEventSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  typedef void(ContractSwitchingSlotSignature)(const boost::posix_time::ptime&,
                                               const Request&,
                                               bool& isSwitched);
  typedef boost::function<ContractSwitchingSlotSignature> ContractSwitchingSlot;
  typedef boost::signals2::connection ContractSwitchingSlotConnection;

  ////////////////////////////////////////////////////////////////////////////////

  explicit Security(Context&,
                    const Lib::Symbol&,
                    MarketDataSource&,
                    const SupportedLevel1Types&);
  virtual ~Security();

  TRDK_CORE_API friend std::ostream& operator<<(std::ostream&, const Security&);

  //! Returns Market Data Source object which provides market data for
  //! this security object.
  const MarketDataSource& GetSource() const;

  RiskControlSymbolContext& GetRiskControlContext(const TradingMode&);

  const InstanceId& GetInstanceId() const;

  //! Returns true if security receives market data right now.
  /** @sa IsTradingSessionOpened
   */
  virtual bool IsOnline() const;

  //! Returns true if trading session is active at this moment.
  /** @sa IsOnline
   */
  virtual bool IsTradingSessionOpened() const;

  //! Sets requested data start time if it is not later than existing.
  void SetRequest(const Request&);
  //! Returns requested data start.
  const Request& GetRequest() const;

  size_t GetNumberOfItemsPerQty() const;
  const Qty& GetLotSize() const;

  uintmax_t GetPricePrecisionPower() const;
  uint8_t GetPricePrecision() const noexcept;

  Price GetPip() const { return 1.0 / GetPricePrecisionPower(); }

  boost::posix_time::ptime GetLastMarketDataTime() const;
  size_t TakeNumberOfMarketDataUpdates() const;

  virtual Price GetLastPrice() const;
  Qty GetLastQty() const;

  Price GetAskPrice() const;
  Price GetAskPriceValue() const;
  Qty GetAskQty() const;
  Qty GetAskQtyValue() const;

  Price GetBidPrice() const;
  Price GetBidPriceValue() const;
  Qty GetBidQty() const;
  Qty GetBidQtyValue() const;

  Qty GetTradedVolume() const;

  //! Returns next expiration time.
  /**
   * Throws exception if expiration is not provided.
   * @sa HasExpiration
   */
  virtual const Lib::ContractExpiration& GetExpiration() const;
  //! Returns true if security has expiration.
  /**
   * Throws exception if expiration is not provided.
   * @sa GetExpiration
   */
  bool HasExpiration() const;

  //! Subscribes to contract switching event.
  /**
   * Notification should be synchronous. Notification generator will wait until
   * the event will be handled by each subscriber.
   */
  ContractSwitchingSlotConnection SubscribeToContractSwitching(
      const ContractSwitchingSlot&) const;

  Level1UpdateSlotConnection SubscribeToLevel1Updates(
      const Level1UpdateSlot&) const;

  Level1UpdateSlotConnection SubscribeToLevel1Ticks(
      const Level1TickSlot&) const;

  NewTradeSlotConnection SubscribeToTrades(const NewTradeSlot&) const;

  BrokerPositionUpdateSlotConnection SubscribeToBrokerPositionUpdates(
      const BrokerPositionUpdateSlot&) const;

  NewBarSlotConnection SubscribeToBars(const NewBarSlot&) const;

  BookUpdateTickSlotConnection SubscribeToBookUpdateTicks(
      const BookUpdateTickSlot&) const;

  //! Subscribes to security service event.
  /**
   * Notification should be synchronous. Notification generator will wait until
   * the event will be handled by each subscriber.
   */
  ServiceEventSlotConnection SubscribeToServiceEvents(
      const ServiceEventSlot&) const;

 protected:
  //! Sets security online and generate event about it.
  /**
   * @sa IsOnline
   * @return True, if state is changed, false - if state was the same before.
   */
  bool SetOnline(const boost::posix_time::ptime&, bool isOnline);
  void SetTradingSessionState(const boost::posix_time::ptime&, bool isOpened);
  void SwitchTradingSession(const boost::posix_time::ptime&);

  bool IsLevel1Required() const;
  bool IsLevel1UpdatesRequired() const;
  bool IsLevel1TicksRequired() const;
  bool IsTradesRequired() const;
  bool IsBrokerPositionRequired() const;
  bool IsBarsRequired() const;

  //! Sets one Level I parameter.
  /**
   * Subscribers will be notified about Level I Update only if parameter will
   * bee changed.
   */
  void SetLevel1(const boost::posix_time::ptime&,
                 const Level1TickValue&,
                 const Lib::TimeMeasurement::Milestones&);
  //! Sets one Level I parameter.
  /**
   * Subscribers will be notified about Level I Update only if parameter will
   * bee changed.
   * @return Returns true value was changed, false if set the same value as
   *         was.
   */
  bool SetLevel1(const boost::posix_time::ptime&,
                 const Level1TickValue&,
                 bool flush,
                 bool isPreviouslyChanged,
                 const Lib::TimeMeasurement::Milestones&);
  //! Sets two Level I parameters and one operation.
  /**
   * More optimal than call "set one parameter" two times. Subscribers will be
   * notified about Level I Update only if parameter will be changed.
   */
  void SetLevel1(const boost::posix_time::ptime&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Lib::TimeMeasurement::Milestones&);
  //! Sets three Level I parameters and one operation.
  /**
   * More optimal than call "set one parameter" three times. Subscribers will be
   * notified about Level I Update only if parameter will bee  changed.
   */
  void SetLevel1(const boost::posix_time::ptime&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Lib::TimeMeasurement::Milestones&);
  //! Sets four Level I parameters and one operation.
  /**
   * More optimal than call "set one parameter" four times. Subscribers will be
   * notified about Level I Update only if parameter will bee changed.
   */
  void SetLevel1(const boost::posix_time::ptime&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Level1TickValue&,
                 const Lib::TimeMeasurement::Milestones&);
  //! Sets many Level I parameters and one operation.
  /** More optimal than call "set one parameter" several times.
   * Subscribers will be notified about Level I Update only if parameter will
   * bee changed.
   */
  void SetLevel1(const boost::posix_time::ptime&,
                 const std::vector<Level1TickValue>&,
                 const Lib::TimeMeasurement::Milestones&);

  //! Adds one Level I parameter tick.
  /**
   * Subscribers will be notified about Level I Update only if parameter will
   * bee changed. Level I Tick event will be generated in any case.
   */
  bool AddLevel1Tick(const boost::posix_time::ptime&,
                     const Level1TickValue&,
                     bool flush,
                     bool isPreviouslyChanged,
                     const Lib::TimeMeasurement::Milestones&);

  //! Adds one Level I parameter tick.
  /**
   * Subscribers will be notified about Level I Update only if parameter will
   * bee changed. Level I Tick event will be generated in any case.
   */
  void AddLevel1Tick(const boost::posix_time::ptime&,
                     const Level1TickValue&,
                     const Lib::TimeMeasurement::Milestones&);
  //! Adds two Level I parameter ticks.
  /**
   * More optimal than call "add one tick" two times. Subscribers will be
   * notified about Level I Update only if parameter will be changed. Level I
   * Tick event will be generated in any case. First tick will be added first in
   * the notification queue, last - will be added last. All ticks can have one
   * type or various.
   */
  void AddLevel1Tick(const boost::posix_time::ptime&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Lib::TimeMeasurement::Milestones&);
  //! Adds three Level I parameter ticks.
  /**
   * More optimal than call "add one tick" three times. Subscribers will
   * be notified about Level I Update only if parameter will be changed.
   * Level I Tick event will be generated in any case. First tick will be
   * added first in the notification queue, last - will be added last.
   * All ticks can have one type or various.
   */
  void AddLevel1Tick(const boost::posix_time::ptime&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Lib::TimeMeasurement::Milestones&);
  //! Adds four Level I parameter ticks.
  /**
   * More optimal than call "add one tick" three times. Subscribers will
   * be notified about Level I Update only if parameter will be changed.
   * Level I Tick event will be generated in any case. First tick will be
   * added first in the notification queue, last - will be added last.
   * All ticks can have one type or various.
   */
  void AddLevel1Tick(const boost::posix_time::ptime&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Level1TickValue&,
                     const Lib::TimeMeasurement::Milestones&);
  //! Adds many Level I parameter ticks.
  /**
   * More optimal than call "add one tick" several times. Subscribers
   * will be notified about Level I Update only if parameter will be
   * changed. Level I Tick event will be generated in any case. First
   * tick will be added first in the notification queue, last - will be
   * added last. All ticks can have one type or various.
   */
  void AddLevel1Tick(const boost::posix_time::ptime&,
                     const std::vector<Level1TickValue>&,
                     const Lib::TimeMeasurement::Milestones&);

  void AddTrade(const boost::posix_time::ptime&,
                const Price&,
                const Qty&,
                const Lib::TimeMeasurement::Milestones&,
                bool useAsLastTrade);

  void UpdateBar(const Bar&);

  //! Sets security broker position info.
  /**
   * Subscribers will be notified only if parameter will be changed.
   * @param[in] isLong     If true - position has type "long", "short"
   *                       otherwise.
   * @param[in] qty        Position size.
   * @param[in] volume     Position volume.
   * @param[in] isInitial  true if it initial data at start.
   */
  void SetBrokerPosition(bool isLong,
                         const Qty& qty,
                         const Volume& volume,
                         bool isInitial);

  //! Sets price book snapshot with current data, but new time.
  /**
   * Not thread-safe, caller should provide thread synchronization.
   * @param book New price book. Object can be used as temporary buffer and
   *             changed by this call, may have wrong data and state after.
   * @param timeMeasurement Time measurement object.
   */
  void SetBook(PriceBook& book,
               const Lib::TimeMeasurement::Milestones& timeMeasurement);

 public:
  //! Sets new expiration.
  /**
   * All marked data for security will be reset (so if security just
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
  bool SetExpiration(const boost::posix_time::ptime& time,
                     const Lib::ContractExpiration&& expiration);
  //! Sets new expiration.
  /**
   * All marked data for security will be reset (so if security just
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
  bool SetExpiration(const boost::posix_time::ptime& time,
                     const Lib::ContractExpiration& expiration);
  //! Continue contract switching which started by signal "contract switching
  //! signal".
  void ContinueContractSwitchingToNextExpiration();

  void StartBars(const boost::posix_time::ptime& startTime,
                 const boost::posix_time::time_duration& barSize);
  void StopBars(const boost::posix_time::time_duration& barSize);

 protected:
  void SetBarsStartTime(const boost::posix_time::time_duration&,
                        const boost::posix_time::ptime&);
  std::vector<
      std::pair<boost::posix_time::time_duration, boost::posix_time::ptime>>
  GetStartedBars() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk
