/**************************************************************************
 *   Created: 2016/09/12 21:41:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContextDummy.hpp"
#include "RiskControl.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace Lib;
using namespace Tests;
namespace lt = boost::local_time;
namespace ids = boost::uuids;
namespace ptr = boost::property_tree;

namespace {
const lt::time_zone_ptr timeZone =
    boost::make_shared<lt::posix_time_zone>("GMT");
Context::Log contextLog(timeZone);
Context::TradingLog tradingLog(timeZone);
}  // namespace

Dummies::Context::Context()
    : trdk::Context(contextLog, tradingLog, Settings()) {}

Dummies::Context &Dummies::Context::GetInstance() {
  static Dummies::Context result;
  return result;
}

std::unique_ptr<Dummies::Context::DispatchingLock>
Dummies::Context::SyncDispatching() const {
  throw std::logic_error("Not supported");
}

RiskControl &Dummies::Context::GetRiskControl(const TradingMode &) {
  static RiskControl result(*this,
                            ptr::ptree().add("RiskControl.isEnabled", false),
                            numberOfTradingModes);
  return result;
}
const RiskControl &Dummies::Context::GetRiskControl(
    const TradingMode &mode) const {
  return const_cast<Dummies::Context *>(this)->GetRiskControl(mode);
}

const ExpirationCalendar &Dummies::Context::GetExpirationCalendar() const {
  throw std::logic_error("Not supported");
}

bool Dummies::Context::HasExpirationCalendar() const {
  throw std::logic_error("Not supported");
}

boost::unordered_set<std::string> Dummies::Context::GetSymbolListHint() const {
  throw std::logic_error("Not supported");
}

size_t Dummies::Context::GetNumberOfMarketDataSources() const {
  throw std::logic_error("Not supported");
}

const MarketDataSource &Dummies::Context::GetMarketDataSource(size_t) const {
  throw std::logic_error("Not supported");
}

MarketDataSource &Dummies::Context::GetMarketDataSource(size_t) {
  throw std::logic_error("Not supported");
}

void Dummies::Context::ForEachMarketDataSource(
    const boost::function<void(const MarketDataSource &)> &) const {
  throw std::logic_error("Not supported");
}

void Dummies::Context::ForEachMarketDataSource(
    const boost::function<void(MarketDataSource &)> &) {
  throw std::logic_error("Not supported");
}

size_t Dummies::Context::GetNumberOfTradingSystems() const { return 0; }

const TradingSystem &Dummies::Context::GetTradingSystem(
    size_t, const TradingMode &) const {
  throw std::logic_error("Not supported");
}

TradingSystem &Dummies::Context::GetTradingSystem(size_t, const TradingMode &) {
  throw std::logic_error("Not supported");
}

DropCopy *Dummies::Context::GetDropCopy() const {
  throw std::logic_error("Not supported");
}

Strategy &Dummies::Context::GetStrategy(const ids::uuid &) {
  throw std::logic_error("Not supported");
}

const Strategy &Dummies::Context::GetStrategy(const ids::uuid &) const {
  throw std::logic_error("Not supported");
}

void Dummies::Context::CloseStrategiesPositions() {
  throw std::logic_error("Not supported");
}

std::unique_ptr<Dummies::Context::AddingTransaction>
Dummies::Context::StartAdding() {
  throw std::logic_error("Not supported");
}
