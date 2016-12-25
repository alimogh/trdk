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
#include "MockMarketDataSource.hpp"
#include "DummyContext.hpp"
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"

using namespace trdk::Tests;

MockMarketDataSource::MockMarketDataSource()
	: MarketDataSource(0, DummyContext::GetInstance(), "0") {
	//...//
}

MockMarketDataSource::~MockMarketDataSource() {
	//...//
}
