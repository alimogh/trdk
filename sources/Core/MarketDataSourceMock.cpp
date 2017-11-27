/*******************************************************************************
 *   Created: 2016/02/07 14:13:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MarketDataSourceMock.hpp"
#include "ContextDummy.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"

using namespace trdk::Tests;

Mocks::MarketDataSource::MarketDataSource()
    : trdk::MarketDataSource(Dummies::Context::GetInstance(), "0") {}
