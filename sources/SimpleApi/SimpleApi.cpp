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

double _stdcall trdk_GetImpliedVolatilityLast(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			"trdk_GetImpliedVolatilityLast result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Last for FOP Symbol"
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

double _stdcall trdk_GetImpliedVolatilityAsk(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetAskImpliedVolatility();
		result *= 100;
		Log::Debug(
			"trdk_GetImpliedVolatilityAsk result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Ask for FOP Symbol"
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

double _stdcall trdk_GetImpliedVolatilityBid(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetBidImpliedVolatility();
		result *= 100;
		Log::Debug(
			"trdk_GetImpliedVolatilityBid result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Bid for FOP Symbol"
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

int _stdcall trdk_GetFopLastQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetLastQty();
		Log::Debug(
			"trdk_GetFopLastQty result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Last Qty for FOP Symbol"
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

double _stdcall trdk_GetFopLastPrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetLastPrice();
		Log::Debug(
			"trdk_GetFopLastPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Last Price for FOP Symbol"
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

int _stdcall trdk_GetFopBidQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetBidQty();
		Log::Debug(
			"trdk_GetFopBidQty result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Bid Qty for FOP Symbol"
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

double _stdcall trdk_GetFopBidPrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetBidPrice();
		Log::Debug(
			"trdk_GetFopBidPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Bid Price for FOP Symbol"
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

int _stdcall trdk_GetFopAskQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetAskQty();
		Log::Debug(
			"trdk_GetFopAskQty result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Ask Qty for FOP Symbol"
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

double _stdcall trdk_GetFopAskPrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
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
			.GetAskPrice();
		Log::Debug(
			"trdk_GetFopAskPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
			symbol,
			expirationDate,
			strike,
			right,
			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Ask Price for FOP Symbol"
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

void _stdcall trdk_SetImpliedVolatilityUpdatePeriod(int updatePeriod) {
	AssertLt(0, updatePeriod);
	try {
		InitDebugLog();
		trdk::Security::SetImpliedVolatilityUpdatePeriodSec(updatePeriod);
		Log::Debug("trdk_SetImpliedVolatilityUpdatePeriod %1%.", updatePeriod);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to Set Implied Volatility Update Period %1%: \"%2%\".",
			updatePeriod,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
}

double _stdcall GetImpliedVolatilityLast(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetImpliedVolatilityLast(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

double _stdcall GetImpliedVolatilityAsk(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetImpliedVolatilityAsk(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

double _stdcall GetImpliedVolatilityBid(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetImpliedVolatilityBid(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

int GetFopLastTradeQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopLastQty(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

double GetFopLastTradePrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopLastPrice(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

int GetFopLastBidQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopBidQty(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

double GetFopLastBidPrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopBidPrice(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

int GetFopLastAskQty(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopAskQty(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

double GetFopLastAskPrice(
			const char *symbol,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass) {
	return trdk_GetFopAskPrice(
		symbol,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass);
}

void SetImpliedVolatilityUpdatePeriod(int updatePeriod) {
	trdk_SetImpliedVolatilityUpdatePeriod(updatePeriod);
}

////////////////////////////////////////////////////////////////////////////////
