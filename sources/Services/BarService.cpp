/**************************************************************************
 *   Created: 2016/09/20 22:21:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once
#include "Prec.hpp"
#include "BarService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

BarService::Error::Error(const char *what) throw()
	: Exception(what) {
	//...//
}

BarService::BarDoesNotExistError::BarDoesNotExistError(const char *what) throw()
	: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

BarService::Bar::Bar()
	: maxAskPrice(0)
	, openAskPrice(0)
	, closeAskPrice(0)
	, minBidPrice(0)
	, openBidPrice(0)
	, closeBidPrice(0)
	, openTradePrice(0)
	, closeTradePrice(0)
	, highTradePrice(0)
	, lowTradePrice(0)
	, tradingVolume(0) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

BarService::BarService(
		Context &context,
		const boost::uuids::uuid &typeId,
		const std::string &implementationName,
		const std::string &instanceName,
		const IniSectionRef &conf)
	: Base(context, typeId, implementationName, instanceName, conf) {
	//...//
}

BarService::~BarService() {
	//...//
}

//////////////////////////////////////////////////////////////////////////
