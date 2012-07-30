/**************************************************************************
 *   Created: 2012/07/15 13:22:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

//////////////////////////////////////////////////////////////////////////

MarketDataSource::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

MarketDataSource::ConnectError::ConnectError()
		: Error("Failed to connect to Market Data Source") {
	//...//
}

//////////////////////////////////////////////////////////////////////////

MarketDataSource::MarketDataSource() {
	//...//
}

MarketDataSource::~MarketDataSource() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

LiveMarketDataSource::LiveMarketDataSource() {
	//...//
}

LiveMarketDataSource::~LiveMarketDataSource() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

HistoryMarketDataSource::HistoryMarketDataSource() {
	//...//
}

HistoryMarketDataSource::~HistoryMarketDataSource() {
	//...//
}

//////////////////////////////////////////////////////////////////////////
