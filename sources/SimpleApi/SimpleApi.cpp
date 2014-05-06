/**************************************************************************
 *   Created: 2014/04/29 22:59:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiBridgeServer.hpp"
#include "SimpleApiBridge.hpp"
#include "SimpleApiUtil.hpp"
#include "Services/BarService.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

////////////////////////////////////////////////////////////////////////////////

namespace {

	BridgeServer theBridgeServer;
	
	const BridgeServer::BridgeId bridgeId = 0;

}

////////////////////////////////////////////////////////////////////////////////

int _stdcall trdk_InitLog(char *logFilePath) {
	try {
		theBridgeServer.InitLog(logFilePath);
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to init log: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t trdk_InitLogToStdOut() {
	try {
		Log::EnableEventsToStdOut();
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to init log to StdOut: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int32_t _stdcall trdk_DestroyAllBridges() {
	try {
		theBridgeServer.DestoryAllBridge();
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to destroy all Bridges: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

uint64_t _stdcall trdk_ResolveFutOpt(
			const char *symbol,
			const char *exchange,
			const char *expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			int32_t dataStartDate,
			int32_t dataStartTime,
			int32_t barIntervalType) {
	Assert(symbol);
	Assert(exchange);
	Assert(expirationDate);
	AssertNe(.0, strike);
	Assert(right);
	Assert(dataStartDate);
	Assert(dataStartTime);
	Assert(tradingClass);
	try {
		const std::string exchangeStr(exchange);
		return theBridgeServer
			.CheckBridge(bridgeId, exchangeStr)
			.ResolveFutOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				Util::ConvertEasyLanguageDateTimeToPTime(
					dataStartDate,
					dataStartTime),
				barIntervalType);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to resolve FOP Symbol \"%1%:%2%\" (%3%, %4%)"
				" across Bridge: \"%5%\".",
			symbol,
			exchange,
			expirationDate,
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

double _stdcall trdk_GetImpliedVolatility(
			uint64_t barServiceHandle,
			int32_t date,
			int32_t time) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetBarService(reinterpret_cast<Bridge::BarServiceHandle &>(barServiceHandle))
			.GetBar(Util::ConvertEasyLanguageDateTimeToPTime(date, time))
			.impliedVolatility;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility across Bridge: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
