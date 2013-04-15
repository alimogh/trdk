/**************************************************************************
 *   Created: 2012/07/15 13:22:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;

//////////////////////////////////////////////////////////////////////////

MarketDataSource::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

MarketDataSource::ConnectError::ConnectError() throw()
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
