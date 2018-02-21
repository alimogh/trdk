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
#include "Fwd.hpp"

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

typedef trdk::Lib::BusinessNumeric<
    double,
    trdk::Lib::Detail::DoubleWithFixedPrecisionNumericPolicy<100000000>>
    Qty;
typedef trdk::Lib::BusinessNumeric<
    double,
    trdk::Lib::Detail::DoubleWithFixedPrecisionNumericPolicy<100000000>>
    Price;
typedef trdk::Lib::BusinessNumeric<
    double,
    trdk::Lib::Detail::DoubleWithFixedPrecisionNumericPolicy<100000000>>
    Volume;

////////////////////////////////////////////////////////////////////////////////

//! Order side.
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

enum PositionSide {
  POSITION_SIDE_LONG,
  POSITION_SIDE_SHORT,
  numberOfPositionSides
};

TRDK_CORE_API const char *ConvertToPch(const trdk::PositionSide &);

inline std::ostream &operator<<(std::ostream &os,
                                const trdk::PositionSide &positionSide) {
  return os << trdk::ConvertToPch(positionSide);
}

////////////////////////////////////////////////////////////////////////////////

//! Time in Force
enum TimeInForce {
  // Good Till Day.
  TIME_IN_FORCE_DAY,
  // Good Till Cancel.
  TIME_IN_FORCE_GTC,
  // At the Opening.
  TIME_IN_FORCE_OPG,
  // Immediate or Cancel.
  TIME_IN_FORCE_IOC,
  // Fill or Kill.
  TIME_IN_FORCE_FOK,
  numberOfTimeInForces
};

TRDK_CORE_API const char *ConvertToPch(const trdk::TimeInForce &);

inline std::ostream &operator<<(std::ostream &os,
                                const trdk::TimeInForce &tif) {
  return os << trdk::ConvertToPch(tif);
}

//! Extended order parameters.
struct OrderParams {
  //! Account.
  boost::optional<std::string> account;

  //! Good in Time.
  /** The order should be canceled if is not filled after this time.
   */
  boost::optional<boost::posix_time::time_duration> goodInTime;

  //! Define forced expiration for order contract.
  /** If set - this expiration will be used, not from security object.
   */
  const trdk::Lib::ContractExpiration *expiration;

  //! Trading system must try to work with position started by specified order
  //! or used by specified order.
  const trdk::OrderTransactionContext *position;

  //! Price that triggers a atop order.
  boost::optional<trdk::Price> stopPrice;

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &,
                                                const trdk::OrderParams &);
};

////////////////////////////////////////////////////////////////////////////////

enum OrderStatus {
  //! Order sent by the owner to the trading system. State of the order is
  //! unknown.
  ORDER_STATUS_SENT,
  //! Order sent to the trading system and received reception confirmation. The
  //! order is active, there are no any trades yet.
  ORDER_STATUS_OPENED,
  //! The order is canceled by the owner with or without partial filling. The
  //! order is not active anymore.
  ORDER_STATUS_CANCELED,
  //! The order is fully filled and is not active anymore. Remaining quantity is
  //! zero.
  ORDER_STATUS_FILLED_FULLY,
  //! The order got a new part of the partial filling and still be active.
  //! Has remaining quantity.
  ORDER_STATUS_FILLED_PARTIALLY,
  //! The order is rejected by the trading system and is not active. Remaining
  //! quantity is canceled.
  ORDER_STATUS_REJECTED,
  //! The unknown error has occurred. State of the order is unknown.
  ORDER_STATUS_ERROR,
  //! Number of order statuses.
  numberOfOrderStatuses
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
  CLOSE_REASON_NONE,
  CLOSE_REASON_SIGNAL,
  CLOSE_REASON_TAKE_PROFIT,
  CLOSE_REASON_TRAILING_STOP,
  CLOSE_REASON_STOP_LOSS,
  CLOSE_REASON_STOP_LIMIT,
  CLOSE_REASON_TIMEOUT,
  CLOSE_REASON_SCHEDULE,
  CLOSE_REASON_ROLLOVER,
  //! Closed by request, not by logic or error.
  CLOSE_REASON_REQUEST,
  //! Closed by engine stop.
  CLOSE_REASON_ENGINE_STOP,
  CLOSE_REASON_OPEN_FAILED,
  CLOSE_REASON_SYSTEM_ERROR,
  numberOfCloseReasons
};

TRDK_CORE_API const char *ConvertToPch(const trdk::CloseReason &);
inline std::ostream &operator<<(std::ostream &os,
                                const trdk::CloseReason &closeReason) {
  return os << trdk::ConvertToPch(closeReason);
}

inline bool IsPassive(const CloseReason &reason) {
  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (reason) {
    case CLOSE_REASON_NONE:
    case CLOSE_REASON_SIGNAL:
    case CLOSE_REASON_TAKE_PROFIT:
    case CLOSE_REASON_TRAILING_STOP:
    case CLOSE_REASON_STOP_LOSS:
    case CLOSE_REASON_STOP_LIMIT:
      return true;
    default:
      return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
}  // namespace trdk

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

  const trdk::Lib::Double &GetValue() const { return m_value; }

 private:
  Level1TickType m_type;
  trdk::Lib::Double m_value;
};
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

enum TradingMode {
  TRADING_MODE_LIVE,
  TRADING_MODE_PAPER,
  TRADING_MODE_BACKTESTING,
  numberOfTradingModes
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
}  // namespace trdk

//////////////////////////////////////////////////////////////////////////

namespace trdk {
//! Order ID.
class OrderId {
 public:
  OrderId() = default;
  OrderId(const std::string &value) : m_value(value) {}
  OrderId(const std::string &&value) : m_value(std::move(value)) {}
  OrderId(int32_t value) : m_value(boost::lexical_cast<std::string>(value)) {}
  OrderId(uint32_t value) : m_value(boost::lexical_cast<std::string>(value)) {}
  OrderId(intmax_t value) : m_value(boost::lexical_cast<std::string>(value)) {}
  OrderId(uintmax_t value) : m_value(boost::lexical_cast<std::string>(value)) {}

  bool operator==(const OrderId &rhs) const { return m_value == rhs.m_value; }
  bool operator!=(const OrderId &rhs) const { return !operator==(rhs); }

  friend std::ostream &operator<<(std::ostream &os,
                                  const trdk::OrderId &orderId) {
    return os << orderId.m_value;
  }
  friend std::istream &operator>>(std::istream &is, trdk::OrderId &orderId) {
    return is >> orderId.m_value;
  }

  const std::string &GetValue() const { return m_value; }

 private:
  std::string m_value;
};
}  // namespace trdk

namespace stdext {

inline size_t hash_value(const trdk::OrderId &orderId) {
  return boost::hash_value(orderId.GetValue());
}
}  // namespace stdext

namespace trdk {
inline size_t hash_value(const trdk::OrderId &orderId) {
  return stdext::hash_value(orderId);
}
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////