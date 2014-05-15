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

#ifdef DEV_VER
	namespace {

		void InitDebugLog() {
			if (theBridgeServer.IsLogInited()) {
				return;
			}
			trdk_InitLog("C:/Jts/trdk.log");
		}

	}
#else
	namespace {

		void InitDebugLog() {
			//...//
		}

	}
#endif


////////////////////////////////////////////////////////////////////////////////

int32_t _stdcall trdk_DestroyAllBridges() {
	try {
		InitDebugLog();
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

double _stdcall trdk_GetImpliedVolatility(
			const char *symbol,
			const char *exchange,
			const char *expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	Assert(expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr)
			.ResolveFutOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass)
			.GetLastImpliedVolatility();
		result *= 100;
		Log::Debug(
			"trdk_GetImpliedVolatility result for \"%1% %2% %3%\": %4%.",
			symbol,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
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
                
double _stdcall GetImpliedVolatility(
			const char *symbol,
			const char *exchange,
			const char *expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetImpliedVolatility(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

////////////////////////////////////////////////////////////////////////////////
