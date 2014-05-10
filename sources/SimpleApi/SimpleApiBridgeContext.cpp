/**************************************************************************
 *   Created: 2013/12/22 00:17:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiBridgeContext.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

////////////////////////////////////////////////////////////////////////////////

class BridgeContext::Implementation :private boost::noncopyable {
	//...//
};

////////////////////////////////////////////////////////////////////////////////

BridgeContext::BridgeContext(boost::shared_ptr<const trdk::Lib::Ini> conf)
		: Base(conf, false),
		m_pimpl(new Implementation) {
	//...//
}

BridgeContext::~BridgeContext() {
	delete m_pimpl;
}

Security * BridgeContext::FindSecurity(const Symbol &symbol) {
	Security *security = GetMarketDataSource().FindSecurity(symbol);
	if (!security) {
		security = &GetMarketDataSource().GetSecurity(*this, symbol);
		GetMarketDataSource().SubscribeToSecurities();
	}
	return security;
}

const trdk::Security * BridgeContext::FindSecurity(const Symbol &symbol) const {
	return const_cast<BridgeContext *>(this)->FindSecurity(symbol);
}

////////////////////////////////////////////////////////////////////////////////
