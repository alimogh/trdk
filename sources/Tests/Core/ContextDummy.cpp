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
#include "Core/RiskControl.hpp"
#include "Core/Settings.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace Lib;
using namespace Tests;
using namespace Core;
namespace lt = boost::local_time;
namespace ids = boost::uuids;
namespace ptr = boost::property_tree;

namespace {
const lt::time_zone_ptr timeZone =
    boost::make_shared<lt::posix_time_zone>("GMT");
Context::Log contextLog(timeZone);
Context::TradingLog tradingLog(timeZone);
}  // namespace

ContextDummy::ContextDummy() : Context(contextLog, tradingLog, Settings()) {}

ContextDummy::Context &ContextDummy::GetInstance() {
  static ContextDummy result;
  return result;
}

std::unique_ptr<ContextDummy::DispatchingLock> ContextDummy::SyncDispatching()
    const {
  throw std::logic_error("Not supported");
}

RiskControl &ContextDummy::GetRiskControl(const TradingMode &) {
  static RiskControl result(*this,
                            ptr::ptree().add("RiskControl.isEnabled", false),
                            numberOfTradingModes);
  return result;
}
const RiskControl &ContextDummy::GetRiskControl(const TradingMode &mode) const {
  return const_cast<ContextDummy *>(this)->GetRiskControl(mode);
}

const ExpirationCalendar &ContextDummy::GetExpirationCalendar() const {
  throw std::logic_error("Not supported");
}

bool ContextDummy::HasExpirationCalendar() const {
  throw std::logic_error("Not supported");
}

size_t ContextDummy::GetNumberOfMarketDataSources() const {
  throw std::logic_error("Not supported");
}

const MarketDataSource &ContextDummy::GetMarketDataSource(size_t) const {
  throw std::logic_error("Not supported");
}

MarketDataSource &ContextDummy::GetMarketDataSource(size_t) {
  throw std::logic_error("Not supported");
}

void ContextDummy::ForEachMarketDataSource(
    const boost::function<void(const MarketDataSource &)> &) const {
  throw std::logic_error("Not supported");
}

void ContextDummy::ForEachMarketDataSource(
    const boost::function<void(MarketDataSource &)> &) {
  throw std::logic_error("Not supported");
}

size_t ContextDummy::GetNumberOfTradingSystems() const { return 0; }

const TradingSystem &ContextDummy::GetTradingSystem(size_t,
                                                    const TradingMode &) const {
  throw std::logic_error("Not supported");
}

TradingSystem &ContextDummy::GetTradingSystem(size_t, const TradingMode &) {
  throw std::logic_error("Not supported");
}

DropCopy *ContextDummy::GetDropCopy() const {
  throw std::logic_error("Not supported");
}

Strategy &ContextDummy::GetSrategy(const ids::uuid &) {
  throw std::logic_error("Not supported");
}

const Strategy &ContextDummy::GetSrategy(const ids::uuid &) const {
  throw std::logic_error("Not supported");
}

void ContextDummy::CloseSrategiesPositions() {
  throw std::logic_error("Not supported");
}

void ContextDummy::Add(const boost::property_tree::ptree &) {
  throw std::logic_error("Not supported");
}
