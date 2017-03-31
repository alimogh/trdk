/**************************************************************************
 *   Created: 2014/04/29 23:28:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiEngine.hpp"
#include "Services/BarService.hpp"
#include "Engine/Context.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;
using namespace trdk::Services;


SimpleApi::Engine::Engine(
		const boost::shared_ptr<trdk::Engine::Context> &context,
		const TradingMode &tradingMode,
		size_t tradingSystemIndex)
	: m_context(context)
	, m_tradingMode(tradingMode)
	, m_tradingSystemIndex(tradingSystemIndex) {
	//...//
}

const TradingSystem::Account & SimpleApi::Engine::GetAccount() const {
	return m_context
		->GetTradingSystem(m_tradingSystemIndex, m_tradingMode)
		.GetAccount();
}
