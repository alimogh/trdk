/**************************************************************************
 *   Created: 2012/09/16 12:38:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {

class Context;

class PriceBook;

class Security;

class Balances;

class OrderTransactionContext;
class OrderStatusHandler;
class Trade;

struct Bar;

class TradingSystem;
class MarketDataSource;

class DropCopy;

class Module;
class Consumer;
class Strategy;

typedef size_t RiskControlOperationId;
class RiskControl;
class RiskControlSymbolContext;
class RiskControlScope;

class Operation;
class Position;
class LongPosition;
class ShortPosition;
class PositionReporter;
class Pnl;
class PnlContainer;

class Settings;

class EventsLog;
class ModuleEventsLog;
class TradingLog;
class ModuleTradingLog;

class Timer;
class TimerScope;
}  // namespace trdk
