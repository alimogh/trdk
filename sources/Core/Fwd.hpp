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

class OrderTransactionContext;
class TradingSystem;
class MarketDataSource;

typedef std::int32_t DropCopyStrategyInstanceId;
typedef std::int32_t DropCopyDataSourceInstanceId;
class DropCopy;

class Module;
class Consumer;
class Strategy;
class Observer;
class Service;

class RiskControl;
class RiskControlSymbolContext;
class RiskControlScope;

class Operation;
class Position;
class LongPosition;
class ShortPosition;
class PositionReporter;

class Settings;

class EventsLog;
class ModuleEventsLog;
class TradingLog;
class ModuleTradingLog;

class Timer;
}
