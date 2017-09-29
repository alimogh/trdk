/**************************************************************************
 *   Created: 2012/11/18 11:09:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

typedef trdk::Lib::Double Qty;

typedef trdk::Lib::Double Price;

typedef trdk::Lib::Double Volume;

typedef boost::uint64_t OrderId;

////////////////////////////////////////////////////////////////////////////////

//! Order side
/** https://mbcm.robotdk.com:8443/display/API/Constants
  */
enum OrderSide {
  ORDER_SIDE_BUY = 0,
  ORDER_SIDE_BID = ORDER_SIDE_BUY,
  ORDER_SIDE_LONG = ORDER_SIDE_BUY,
  ORDER_SIDE_SELL = 1,
  ORDER_SIDE_OFFER = ORDER_SIDE_SELL,
  ORDER_SIDE_ASK = ORDER_SIDE_SELL,
  ORDER_SIDE_SHORT = ORDER_SIDE_SELL,
  numberOfOrderSides
};

TRDK_CORE_API const char *ConvertToPch(const trdk::OrderSide &);

inline std::ostream &operator<<(std::ostream &os, const trdk::OrderSide &side) {
  return os << trdk::ConvertToPch(side);
}

////////////////////////////////////////////////////////////////////////////////

//! Time in Force
/** https://mbcm.robotdk.com:8443/display/API/Constants
  */
enum TimeInForce {
  // Good Till Day.
  TIME_IN_FORCE_DAY = 0,
  // Good Till Cancel.
  TIME_IN_FORCE_GTC = 1,
  // At the Opening.
  TIME_IN_FORCE_OPG = 2,
  // Immediate or Cancel.
  TIME_IN_FORCE_IOC = 3,
  // Fill or Kill.
  TIME_IN_FORCE_FOK = 4,
  numberOfTimeInForces
};

//! Extended order parameters.
struct OrderParams {
  //! Account.
  boost::optional<std::string> account;

  //! Display size for Iceberg orders.
  boost::optional<trdk::Qty> displaySize;

  //! Minimum trade quantity. Must be at most the order quantity.
  /** For cache pair could be in different currency.
    */
  boost::optional<trdk::Qty> minTradeQty;

  //! Good Till Time.
  /** Absolute value in Coordinated Universal Time (UTC). Incompatible
    * with goodInSeconds.
    * @sa trdk::OrderParams::goodInSeconds
    */
  boost::optional<boost::posix_time::ptime> goodTillTime;
  //! Good next N seconds.
  /** Incompatible with goodTillTime.
    * @sa trdk::OrderParams::goodTillTime
    */
  boost::optional<uintmax_t> goodInSeconds;

  //! Order ID to replace.
  boost::optional<uintmax_t> orderIdToReplace;

  //! Order sent not by strategy.
  bool isManualOrder;

  //! Defines order quantity precision.
  /** If set - order quantity will be rounded to this precision.
    */
  boost::optional<uint8_t> qtyPrecision;

  //! Define forced expiration for order contract.
  /** If set - this expiration will be used, not from security object.
    */
  const trdk::Lib::ContractExpiration *expiration;

  explicit OrderParams() : isManualOrder(false), expiration(nullptr) {}

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &,
                                                const trdk::OrderParams &);
};

////////////////////////////////////////////////////////////////////////////////

enum OrderStatus {
  ORDER_STATUS_SENT = 100,
  ORDER_STATUS_REQUESTED_CANCEL = 200,
  ORDER_STATUS_SUBMITTED = 300,
  ORDER_STATUS_CANCELLED = 400,
  ORDER_STATUS_FILLED = 500,
  ORDER_STATUS_FILLED_PARTIALLY = 600,
  ORDER_STATUS_REJECTED = 700,
  ORDER_STATUS_INACTIVE = 800,
  ORDER_STATUS_ERROR = 900,
  numberOfOrderStatuses = 9
};

TRDK_CORE_API const char *ConvertToPch(const trdk::OrderStatus &);

inline std::ostream &operator<<(std::ostream &os,
                                const trdk::OrderStatus &status) {
  os << trdk::ConvertToPch(status);
  return os;
}

////////////////////////////////////////////////////////////////////////////////

enum OrderType {
  ORDER_TYPE_LIMIT = 0,
  ORDER_TYPE_MARKET = 1,
  numberOfOrderTypes = 2
};

////////////////////////////////////////////////////////////////////////////////

enum CloseReason {
  CLOSE_REASON_NONE = 0,
  CLOSE_REASON_SIGNAL = 100,
  CLOSE_REASON_TAKE_PROFIT = 200,
  CLOSE_REASON_TRAILING_STOP = 300,
  CLOSE_REASON_STOP_LOSS = 400,
  CLOSE_REASON_TIMEOUT = 500,
  CLOSE_REASON_SCHEDULE = 600,
  CLOSE_REASON_ROLLOVER = 700,
  //! Closed by request, not by logic or error.
  CLOSE_REASON_REQUEST = 800,
  //! Closed by engine stop.
  CLOSE_REASON_ENGINE_STOP = 900,
  CLOSE_REASON_OPEN_FAILED = 1000,
  CLOSE_REASON_SYSTEM_ERROR = 1100,
  numberOfCloseReasons = 12
};

TRDK_CORE_API const char *ConvertToPch(const trdk::CloseReason &);
inline std::ostream &operator<<(std::ostream &os,
                                const trdk::CloseReason &closeReason) {
  return os << trdk::ConvertToPch(closeReason);
}

////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

enum Level1TickType {
  LEVEL1_TICK_LAST_PRICE = 0,
  LEVEL1_TICK_LAST_QTY = 1,
  LEVEL1_TICK_BID_PRICE = 2,
  LEVEL1_TICK_BID_QTY = 3,
  LEVEL1_TICK_ASK_PRICE = 4,
  LEVEL1_TICK_ASK_QTY = 5,
  LEVEL1_TICK_TRADING_VOLUME = 6,
  numberOfLevel1TickTypes = 7
};

template <trdk::Level1TickType type>
struct Level1TickTypeToValueType {};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_LAST_PRICE> {
  typedef trdk::Price Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_LAST_QTY> {
  typedef trdk::Qty Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_BID_PRICE> {
  typedef trdk::Price Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_BID_QTY> {
  typedef trdk::Qty Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_ASK_PRICE> {
  typedef trdk::Price Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_ASK_QTY> {
  typedef trdk::Qty Type;
};
template <>
struct Level1TickTypeToValueType<LEVEL1_TICK_TRADING_VOLUME> {
  typedef trdk::Price Type;
};

TRDK_CORE_API const char *ConvertToPch(const trdk::Level1TickType &);
TRDK_CORE_API trdk::Level1TickType ConvertToLevel1TickType(const std::string &);

class Level1TickValue {
 public:
  explicit Level1TickValue(const Level1TickType &type, double value)
      : m_type(type), m_value(value) {}

  template <trdk::Level1TickType type, typename Value>
  static Level1TickValue Create(const Value &value) {
    return Level1TickValue(type, value);
  }
  template <typename Value>
  static Level1TickValue Create(const trdk::Level1TickType &type,
                                const Value &value) {
    static_assert(numberOfLevel1TickTypes == 7, "List changed.");
    switch (type) {
      case LEVEL1_TICK_LAST_PRICE:
        return Create<LEVEL1_TICK_LAST_PRICE>(value);
      case LEVEL1_TICK_LAST_QTY:
        return Create<LEVEL1_TICK_LAST_QTY>(value);
      case LEVEL1_TICK_BID_PRICE:
        return Create<LEVEL1_TICK_BID_PRICE>(value);
      case LEVEL1_TICK_BID_QTY:
        return Create<LEVEL1_TICK_BID_QTY>(value);
      case LEVEL1_TICK_ASK_PRICE:
        return Create<LEVEL1_TICK_ASK_PRICE>(value);
      case LEVEL1_TICK_ASK_QTY:
        return Create<LEVEL1_TICK_ASK_QTY>(value);
      case LEVEL1_TICK_TRADING_VOLUME:
        return Create<LEVEL1_TICK_TRADING_VOLUME>(value);
    }
    AssertEq(LEVEL1_TICK_LAST_PRICE, type);
    throw Exception("Unknown Level 1 Tick Value Type");
  }

  const Level1TickType &GetType() const { return m_type; }

  double GetValue() const { return m_value; }

 private:
  Level1TickType m_type;
  double m_value;
};
}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

enum TradingMode {
  TRADING_MODE_LIVE = 1,
  TRADING_MODE_PAPER = 2,
  TRADING_MODE_BACKTESTING = 3,
  numberOfTradingModes = 3
};
TRDK_CORE_API trdk::TradingMode ConvertTradingModeFromString(
    const std::string &);
TRDK_CORE_API const std::string &ConvertToString(const trdk::TradingMode &);

enum StopMode {
  STOP_MODE_IMMEDIATELY,
  STOP_MODE_GRACEFULLY_ORDERS,
  STOP_MODE_GRACEFULLY_POSITIONS,
  STOP_MODE_UNKNOWN,
  numberOfStopModes = STOP_MODE_UNKNOWN
};

inline std::ostream &operator<<(std::ostream &oss,
                                const trdk::TradingMode &mode) {
  oss << trdk::ConvertToString(mode);
  return oss;
}
}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

enum OperationResult {
  OPERATION_RESULT_UNCOMPLETED = 0,
  OPERATION_RESULT_PROFIT = 1,
  OPERATION_RESULT_LOSS = 2,
  numberOfOperationResults = 3
};

typedef std::vector<std::pair<trdk::Lib::Currency, Volume>> FinancialResult;
}

//////////////////////////////////////////////////////////////////////////
