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
#include "MqlBridgeContext.hpp"
#include "MqlBridgeStrategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;

////////////////////////////////////////////////////////////////////////////////

class BridgeContext::Implementation :private boost::noncopyable {

public:

	boost::shared_ptr<BridgeStrategy> m_strategy;

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

boost::shared_ptr<BridgeStrategy> BridgeContext::InitStrategy(
			const std::string &tag) {
	if (m_pimpl->m_strategy) {
		throw ModuleError("MQL Bridge Strategy is already created");
	}
	m_pimpl->m_strategy.reset(new BridgeStrategy(*this, tag));
	return m_pimpl->m_strategy;
}

boost::shared_ptr<BridgeStrategy> BridgeContext::GetStrategy() {
	if (!m_pimpl->m_strategy) {
		throw ModuleError("MQL Bridge Strategy is not created");
	}
	return m_pimpl->m_strategy;
}

boost::shared_ptr<const BridgeStrategy> BridgeContext::GetStrategy() const {
	return const_cast<BridgeContext *>(this)->GetStrategy();
}

////////////////////////////////////////////////////////////////////////////////
