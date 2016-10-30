/**************************************************************************
 *   Created: 2016/09/12 21:54:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "DummyMarketDataSource.hpp"
#include "DummyContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

DummyMarketDataSource::DummyMarketDataSource(trdk::Context *context)
	: MarketDataSource(
		0,
		context
			?	*context
			:	DummyContext::GetInstance(),
		"Tag") {
	//...//
}

DummyMarketDataSource::~DummyMarketDataSource() {
	//...//
}

DummyMarketDataSource & DummyMarketDataSource::GetInstance() {
	static DummyMarketDataSource result;
	return result;
}

void DummyMarketDataSource::Connect(const trdk::Lib::IniSectionRef &) {
	throw std::logic_error("Not supported");
}

void DummyMarketDataSource::SubscribeToSecurities() {
	throw std::logic_error("Not supported");
}

trdk::Security & DummyMarketDataSource::CreateNewSecurityObject(
		const trdk::Lib::Symbol &) {
	throw std::logic_error("Not supported");
}
