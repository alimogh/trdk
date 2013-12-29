/**************************************************************************
 *   Created: 2013/12/22 01:44:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MqlBridgeStrategy.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;

BridgeStrategy::BridgeStrategy(
			trdk::Context &context,
			const std::string &tag)
		: Strategy(context, "MqlBridge", tag) {
	//...//
}

BridgeStrategy::~BridgeStrategy() {
	//...//
}

std::string BridgeStrategy::GetRequiredSuppliers() const {
	return std::string();
}

void BridgeStrategy::ReportDecision(const trdk::Position &) const {
	//...//
}

std::auto_ptr<PositionReporter> BridgeStrategy::CreatePositionReporter() const {
	
	class NullPositionReporter : private PositionReporter {
	public:
		NullPositionReporter() {
			//...//
		}
		virtual ~NullPositionReporter() {
			//...//
		}
	public:
		virtual void ReportClosedPositon(const trdk::Position &) {
			//...//
		}
	};

	return std::auto_ptr<PositionReporter>(new NullPositionReporter);

}

void BridgeStrategy::UpdateAlogImplSettings(const trdk::Lib::IniSectionRef &) {
	//...//
}

Symbol BridgeStrategy::GetSymbol(std::string symbol) const {
	boost::erase_all(symbol, ":");
	return Symbol::ParseForex(
		symbol,
		GetContext().GetSettings().GetDefaultExchange());
}

Security & BridgeStrategy::GetSecurity(const std::string &symbol) {
	return GetContext().GetSecurity(GetSymbol(symbol));
}

OrderId BridgeStrategy::OpenLongPosition(
			const std::string &symbol,
			Qty qty,
			double price) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	boost::shared_ptr<Position> position(
		new LongPosition(*this, security, qty, priceScaled));
	return position->Open(priceScaled);
}

OrderId BridgeStrategy::OpenLongPositionByMarketPrice(
			const std::string &symbol,
			Qty qty) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security = GetSecurity(symbol);
	boost::shared_ptr<Position> position(
		new LongPosition(*this, security, qty, 0));
	return position->OpenAtMarketPrice();
}

OrderId BridgeStrategy::OpenShortPosition(
			const std::string &symbol,
			Qty qty,
			double price) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	boost::shared_ptr<Position> position(
		new ShortPosition(*this, security, qty, priceScaled));
	return position->Open(priceScaled);
}

OrderId BridgeStrategy::OpenShortPositionByMarketPrice(
			const std::string &symbol,
			Qty qty) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security = GetSecurity(symbol);
	boost::shared_ptr<Position> position(
		new ShortPosition(*this, security, qty, 0));
	return position->OpenAtMarketPrice();
}

Qty BridgeStrategy::GetPositionQty(const std::string &symbol) const {
	const auto &pos
		= GetContext().GetTradeSystem().GetBrokerPostion(GetSymbol(symbol));
	return pos.qty;
}
