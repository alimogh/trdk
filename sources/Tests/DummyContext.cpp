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
#include "DummyContext.hpp"
#include "Core/Settings.hpp"
#include "Core/RiskControl.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

DummyContext::DummyContext()
	: Context(
		*static_cast<Log *>(nullptr),
		*static_cast<TradingLog *>(nullptr),
		Settings(),
		boost::unordered_map<std::string, std::string>()) {
	//...//
}
		
DummyContext::~DummyContext() {
	//...//
}
	
DummyContext & DummyContext::GetInstance() {
	static DummyContext result;
	return result;
}

std::unique_ptr<DummyContext::DispatchingLock> DummyContext::SyncDispatching()
		const {
	throw std::logic_error("Not supported");
}

RiskControl & DummyContext::GetRiskControl(const TradingMode &) {
	static RiskControl result(
		*this,
		IniString("[RiskControl]\nis_enabled = false"),
		numberOfTradingModes);
	return result;
}
const RiskControl & DummyContext::GetRiskControl(
		const TradingMode &mode)
		const {
	return const_cast<DummyContext *>(this)->GetRiskControl(mode);
}

const ExpirationCalendar & DummyContext::GetExpirationCalendar() const {
	throw std::logic_error("Not supported");
}

size_t DummyContext::GetNumberOfMarketDataSources() const {
	throw std::logic_error("Not supported");
}

const MarketDataSource & DummyContext::GetMarketDataSource(size_t) const {
	throw std::logic_error("Not supported");
}

MarketDataSource & DummyContext::GetMarketDataSource(size_t) {
	throw std::logic_error("Not supported");
}

void DummyContext::ForEachMarketDataSource(
		const boost::function<bool (const MarketDataSource &)> &)
		const {
	throw std::logic_error("Not supported");
}

void DummyContext::ForEachMarketDataSource(
		const boost::function<bool (MarketDataSource &)> &) {
	throw std::logic_error("Not supported");
}

size_t DummyContext::GetNumberOfTradingSystems() const {
	return 0;
}

const TradingSystem & DummyContext::GetTradingSystem(
		size_t,
		const TradingMode &)
		const {
	throw std::logic_error("Not supported");
}

TradingSystem & DummyContext::GetTradingSystem(size_t, const TradingMode &) {
	throw std::logic_error("Not supported");
}

DropCopy * DummyContext::GetDropCopy() const {
	throw std::logic_error("Not supported");
}
