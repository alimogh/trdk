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

void Consumer::OnSecurityStart(Security &) {
	//...//
}

void Consumer::OnLevel1Update(Security &security) {
	GetLog().Error(
		"Subscribed to %1% Level 1 Updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
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

void Consumer::OnServiceDataUpdate(const Service &service) {
	GetLog().Error(
		"Subscribed to \"%1%\", but can't work with it"
			" (hasn't OnServiceDataUpdate method implementation).",
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

void Consumer::OnPositionUpdate(Position &) {
	//...//
}

void Consumer::RegisterSource(Security &security) {
	if (m_pimpl->m_securities.Insert(security)) {
		OnSecurityStart(security);
	}
}

Consumer::SecurityList & Consumer::GetSecurities() {
	return m_pimpl->m_securities;
}

const Consumer::SecurityList & Consumer::GetSecurities() const {
	return const_cast<Consumer *>(this)->GetSecurities();
}

////////////////////////////////////////////////////////////////////////////////
