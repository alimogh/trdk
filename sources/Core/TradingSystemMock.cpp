/*******************************************************************************
 *   Created: 2017/07/30 18:25:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TradingSystemMock.hpp"
#include "ContextDummy.hpp"

using namespace trdk;
using namespace trdk::Tests;

Mocks::TradingSystem::TradingSystem()
    : trdk::TradingSystem(numberOfTradingModes,
                          std::numeric_limits<size_t>::max(),
                          Dummies::Context::GetInstance(),
                          "Mock") {}
