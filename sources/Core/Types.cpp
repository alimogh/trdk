/**************************************************************************
 *   Created: 2013/09/08 03:52:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Types.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const OrderSide &side) {
  static_assert(numberOfOrderSides == 2, "List changed.");
  switch (side) {
    default:
      AssertEq(int(ORDER_SIDE_BUY),
               int(side));  // "to int" - to avoid recursion
      return "undefined";
    case ORDER_SIDE_BUY:
      return "buy";
    case ORDER_SIDE_SELL:
      return "sell";
  }
}

////////////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const PositionSide &type) {
  static_assert(numberOfPositionSides == 2, "List changed.");
  switch (type) {
    default:
      AssertEq(POSITION_SIDE_LONG, type);
      return "undefined";
    case POSITION_SIDE_LONG:
      return "long";
    case POSITION_SIDE_SHORT:
      return "short";
  }
}

////////////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const Level1TickType &tickType) {
  static_assert(numberOfLevel1TickTypes == 7, "List changed.");
  switch (tickType) {
    default:
      AssertEq(int(LEVEL1_TICK_LAST_PRICE),
               int(tickType));  // "to int" - to avoid recursion
      return "undefined";
    case LEVEL1_TICK_LAST_PRICE:
      return "last price";
    case LEVEL1_TICK_LAST_QTY:
      return "last qty";
    case LEVEL1_TICK_BID_PRICE:
      return "bid price";
    case LEVEL1_TICK_BID_QTY:
      return "bid qty";
    case LEVEL1_TICK_ASK_PRICE:
      return "ask price";
    case LEVEL1_TICK_ASK_QTY:
      return "ask qty";
    case LEVEL1_TICK_TRADING_VOLUME:
      return "trading volume";
  }
}

Level1TickType trdk::ConvertToLevel1TickType(const std::string &source) {
  if (source == "last price") {
    return LEVEL1_TICK_LAST_PRICE;
  } else if (source == "last qty") {
    return LEVEL1_TICK_LAST_QTY;
  } else if (source == "bid price") {
    return LEVEL1_TICK_BID_PRICE;
  } else if (source == "bid qty") {
    return LEVEL1_TICK_BID_QTY;
  } else if (source == "ask price") {
    return LEVEL1_TICK_ASK_PRICE;
  } else if (source == "ask qty") {
    return LEVEL1_TICK_ASK_QTY;
  } else if (source == "trading volume") {
    return LEVEL1_TICK_TRADING_VOLUME;
  } else {
    boost::format message("Unknown Level 1 Tick Type \"%1%\"");
    message % source;
    throw Exception(message.str().c_str());
  }
}

////////////////////////////////////////////////////////////////////////////////

TradingMode trdk::ConvertTradingModeFromString(const std::string &mode) {
  static_assert(numberOfTradingModes == 3, "List changed.");

  if (boost::iequals(mode, ConvertToString(TRADING_MODE_PAPER))) {
    return TRADING_MODE_PAPER;
  } else if (boost::iequals(mode, ConvertToString(TRADING_MODE_LIVE))) {
    return TRADING_MODE_LIVE;
  } else {
    boost::format message(
        "Wrong trading mode \"%1%\", allowed: \"%2%\" and \"%3%\".");
    message % mode % ConvertToString(TRADING_MODE_PAPER) %
        ConvertToString(TRADING_MODE_LIVE);
    throw trdk::Lib::Exception(message.str().c_str());
  }
}

namespace {

const char *ConvertToPch(const TradingMode &mode) {
  static_assert(numberOfTradingModes == 3, "List changed.");
  switch (mode) {
    case TRADING_MODE_LIVE:
      return "live";
    case TRADING_MODE_PAPER:
      return "paper";
    case TRADING_MODE_BACKTESTING:
      return "backtesting";
    default:
      AssertEq(int(TRADING_MODE_LIVE),
               int(mode));  // "to int" - to avoid recursion
      return "UNKNOWN";
  }
}

const std::string tradingModeLive = ConvertToPch(TRADING_MODE_LIVE);
const std::string tradingModePaper = ConvertToPch(TRADING_MODE_PAPER);
const std::string tradingModeBacktesting =
    ConvertToPch(TRADING_MODE_BACKTESTING);
const std::string tradingModeUnknown = "UNKNOWN";
}  // namespace

const std::string &trdk::ConvertToString(const TradingMode &mode) {
  static_assert(numberOfTradingModes, "List changed.");
  switch (mode) {
    case TRADING_MODE_LIVE:
      return tradingModeLive;
    case TRADING_MODE_PAPER:
      return tradingModePaper;
    case TRADING_MODE_BACKTESTING:
      return tradingModeBacktesting;
    default:
      AssertEq(int(TRADING_MODE_LIVE),
               int(mode));  // "to int" - to avoid recursion
      return tradingModeUnknown;
  }
}

//////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const OrderStatus &status) {
  static_assert(numberOfOrderStatuses == 7, "List changed.");
  switch (status) {
    default:
      AssertEq(int(ORDER_STATUS_SENT),
               int(status));  // "to int" - to avoid recursion
      return "undefined";
    case ORDER_STATUS_SENT:
      return "sent";
    case ORDER_STATUS_OPENED:
      return "opened";
    case ORDER_STATUS_CANCELED:
      return "canceled";
    case ORDER_STATUS_FILLED_FULLY:
      return "filled";
    case ORDER_STATUS_FILLED_PARTIALLY:
      return "filled partially";
    case ORDER_STATUS_REJECTED:
      return "rejected";
    case ORDER_STATUS_ERROR:
      return "error";
  }
}

////////////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const CloseReason &closeReason) {
  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (closeReason) {
    default:
      AssertEq(int(CLOSE_REASON_NONE),
               int(closeReason));  // "to int" - to avoid recursion
      return "undefined";
    case CLOSE_REASON_NONE:
      return "none";
    case CLOSE_REASON_SIGNAL:
      return "signal";
    case CLOSE_REASON_TAKE_PROFIT:
      return "take-profit";
    case CLOSE_REASON_TRAILING_STOP:
      return "trailing-stop";
    case CLOSE_REASON_STOP_LOSS:
      return "stop-loss";
    case CLOSE_REASON_STOP_LIMIT:
      return "stop-limit";
    case CLOSE_REASON_TIMEOUT:
      return "timeout";
    case CLOSE_REASON_SCHEDULE:
      return "schedule";
    case CLOSE_REASON_ROLLOVER:
      return "rollover";
    case CLOSE_REASON_REQUEST:
      return "request";
    case CLOSE_REASON_ENGINE_STOP:
      return "engine stop";
    case CLOSE_REASON_OPEN_FAILED:
      return "open failed";
    case CLOSE_REASON_SYSTEM_ERROR:
      return "sys error";
  }
}

////////////////////////////////////////////////////////////////////////////////

const char *trdk::ConvertToPch(const TimeInForce &tif) {
  static_assert(numberOfTimeInForces == 6, "List changed.");
  switch (tif) {
    default:
      AssertEq(int(TIME_IN_FORCE_DAY),
               int(tif));  // "to int" - to avoid recursion
      return "unknown";
    case TIME_IN_FORCE_DAY:
      return "day";
    case TIME_IN_FORCE_GTC:
      return "gtc";
    case TIME_IN_FORCE_GTD:
      return "gtd";
    case TIME_IN_FORCE_OPG:
      return "opg";
    case TIME_IN_FORCE_IOC:
      return "ioc";
    case TIME_IN_FORCE_FOK:
      return "foc";
  }
}

////////////////////////////////////////////////////////////////////////////////
