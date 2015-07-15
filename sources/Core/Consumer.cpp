/**************************************************************************
 *   Created: 2013/05/14 07:19:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Consumer.hpp"
#include "ModuleSecurityList.hpp"
#include "Security.hpp"
#include "Service.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

class Consumer::Implementation : private boost::noncopyable {
public:
	ModuleSecurityList m_securities;
};

////////////////////////////////////////////////////////////////////////////////

Consumer::Consumer(
			Context &context,
			const std::string &typeName,
			const std::string &name,
			const std::string &tag)
		: Module(context, typeName, name, tag),
		m_pimpl(new Implementation) {
	//...//
}

Consumer::~Consumer() {
	delete m_pimpl;
}

pt::ptime Consumer::OnSecurityStart(Security &) {
	return pt::not_a_date_time;
}

void Consumer::OnLevel1Tick(
				Security &security,
				const pt::ptime &,
				const Level1TickValue &) {
	GetLog().Error(
		"Subscribed to %1% Level 1 Ticks, but can't work with it"
			" (hasn't OnLevel1Tick method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 Ticks, but can't work with it");
}

void Consumer::OnNewTrade(
					Security &security,
					const pt::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	GetLog().Error(
		"Subscribed to %1% new trades, but can't work with it"
			" (hasn't OnNewTrade method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Consumer::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &) {
	GetLog().Error(
		"Subscribed to \"%1%\", but can't work with it"
			" (hasn't OnServiceDataUpdate method implementation).",
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

void Consumer::OnBrokerPositionUpdate(
			Security &security,
			Qty,
			bool /*isInitial*/) {
	GetLog().Error(
		"Subscribed to %1% Broker Positions Updates, but can't work with it"
			" (hasn't OnBrokerPositionUpdate method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Broker Positions Updates, but can't work with it");
}

void Consumer::OnNewBar(Security &security, const Security::Bar &) {
	GetLog().Error(
		"Subscribed to %1% new bars, but can't work with it"
			" (hasn't OnNewBar method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new bars, but can't work with it");
}

void Consumer::RegisterSource(Security &security) {
	if (!m_pimpl->m_securities.Insert(security)) {
		return;
	}
	const auto dataStart = OnSecurityStart(security);
	if (dataStart != pt::not_a_date_time) {
		security.SetRequestedDataStartTime(dataStart);
	}
}

Consumer::SecurityList & Consumer::GetSecurities() {
	return m_pimpl->m_securities;
}

const Consumer::SecurityList & Consumer::GetSecurities() const {
	return const_cast<Consumer *>(this)->GetSecurities();
}

////////////////////////////////////////////////////////////////////////////////
