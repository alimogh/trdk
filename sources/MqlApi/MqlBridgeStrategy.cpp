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
	return std::string("Broker Positions[EUR:FOREX:IDEALPRO:USD]");
}

void BridgeStrategy::OnBrokerPositionUpdate(
			Security &/*security*/,
			Qty /*qty*/,
			bool /*isInitial*/) {
	//...//
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

OrderId BridgeStrategy::OpenLongPosition(Qty qty, double price) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security
		= GetContext().GetSecurity(
			Symbol::Parse("EUR", "IDEALPRO", "FOREX", "USD"));
	const auto priceScaled = security.ScalePrice(price);
	boost::shared_ptr<Position> position(
		new LongPosition(*this, security, qty, priceScaled));
	return position->Open(priceScaled);
}

OrderId BridgeStrategy::OpenShortPosition(Qty qty, double price) {
	AssertLt(0, qty);
	const Lock lock(GetMutex());
	Security &security
		= GetContext().GetSecurity(
			Symbol::Parse("EUR", "IDEALPRO", "FOREX", "USD"));
	const auto priceScaled = security.ScalePrice(price);
	boost::shared_ptr<Position> position(
		new ShortPosition(*this, security, qty, priceScaled));
	return position->Open(priceScaled);
}

void BridgeStrategy::CloseAllPositions() {
	const Lock lock(GetMutex());
	const auto &end = GetPositions().GetEnd();
	for (auto i = GetPositions().GetBegin(); i != end; ++i) {
		i->CancelAtMarketPrice(Position::CLOSE_TYPE_NONE);
	}
}
