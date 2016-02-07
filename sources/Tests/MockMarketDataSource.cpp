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
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"

using namespace trdk::Tests;

MockMarketDataSource::MockMarketDataSource(
		size_t index,
		trdk::Context &context,
		const std::string &tag)
	: MarketDataSource(index, context, tag) {
	//...//
}

MockMarketDataSource::~MockMarketDataSource() {
	//...//
}
