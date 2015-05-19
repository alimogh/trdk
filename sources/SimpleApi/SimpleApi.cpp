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

		static bool isCallsDebugingOn = false;

		void InitDebugLog() {
			if (theBridgeServer.IsLogInited()) {
				return;
			}
			trdk_InitLog("C:/Jts/trdk.log");
			isCallsDebugingOn = true;
		}

		bool CheckCallsDebuging() {
			if (isCallsDebugingOn) {
				isCallsDebugingOn = false;
				return true;
			} else {
				return false;
			}
		}

	}
#else
	namespace {

		void InitDebugLog() {
			//...//
		}

		bool CheckCallsDebuging() {
			return false;
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
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetLastImpliedVolatility();
		result *= 100;
		if (CheckCallsDebuging()) {
 			Log::Debug(
 				"trdk_GetImpliedVolatilityLast result for \"%1%:%2%:%3%:%4%\": %5%.",
 				symbol,
 				boost::lexical_cast<std::string>(expirationDate),
 				strike,
 				right,
 				result);
		}
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Last for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double _stdcall trdk_GetImpliedVolatilityAsk(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetAskImpliedVolatility();
		result *= 100;
// 		Log::Debug(
// 			"trdk_GetImpliedVolatilityAsk result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Ask for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double _stdcall trdk_GetImpliedVolatilityBid(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetBidImpliedVolatility();
		result *= 100;
// 		Log::Debug(
// 			"trdk_GetImpliedVolatilityBid result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Implied Volatility Bid for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int _stdcall trdk_GetFopLastQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetLastQty();
// 		Log::Debug(
// 			"trdk_GetFopLastQty result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Last Qty for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double _stdcall trdk_GetFopLastPrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetLastPrice();
// 		Log::Debug(
// 			"trdk_GetFopLastPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Last Price for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int _stdcall trdk_GetFopBidQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetBidQty();
// 		Log::Debug(
// 			"trdk_GetFopBidQty result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Bid Qty for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double _stdcall trdk_GetFopBidPrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetBidPrice();
// 		Log::Debug(
// 			"trdk_GetFopBidPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Bid Price for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int _stdcall trdk_GetFopAskQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetAskQty();
// 		Log::Debug(
// 			"trdk_GetFopAskQty result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Ask Qty for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double _stdcall trdk_GetFopAskPrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	Assert(symbol);
	Assert(exchange);
	AssertLt(.0, expirationDate);
	AssertLt(.0, strike);
	Assert(right);
	Assert(tradingClass);
	Assert(optType);
	try {
		InitDebugLog();
		const std::string exchangeStr(exchange);
		auto result = theBridgeServer
			.CheckBridge(bridgeId, exchangeStr, false)
			.ResolveOpt(
				symbol,
				exchangeStr,
				expirationDate,
				strike,
				right,
				tradingClass,
				optType,
				currency)
			.GetAskPrice();
// 		Log::Debug(
// 			"trdk_GetFopAskPrice result for \"%1%:%2%:%3%:%4%\": %5%.",
// 			symbol,
// 			boost::lexical_cast<std::string>(expirationDate),
// 			strike,
// 			right,
// 			result);
		return result;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Ask Price for FOP Symbol"
				" \"%1%:%2%\" (%3%, %4%): \"%5%\".",
			symbol,
			exchange,
			boost::lexical_cast<std::string>(expirationDate),
			strike,
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

void _stdcall trdk_SetImpliedVolatilityUpdatePeriod(int updatePeriod) {
	AssertLe(0, updatePeriod);
	try {
		InitDebugLog();
		trdk::Security::SetImpliedVolatilityUpdatePeriodSec(updatePeriod);
/*		Log::Debug("trdk_SetImpliedVolatilityUpdatePeriod %1%.", updatePeriod);*/
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
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetImpliedVolatilityLast(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

double _stdcall GetImpliedVolatilityAsk(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetImpliedVolatilityAsk(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

double _stdcall GetImpliedVolatilityBid(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetImpliedVolatilityBid(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

int GetFopLastTradeQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopLastQty(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

double GetFopLastTradePrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopLastPrice(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

int GetFopLastBidQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopBidQty(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

double GetFopLastBidPrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopBidPrice(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

int GetFopLastAskQty(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopAskQty(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

double GetFopLastAskPrice(
			const char *symbol,
			const char *currency,
			const char *exchange,
			double expirationDate,
			double strike,
			const char *right,
			const char *tradingClass,
			const char *optType) {
	return trdk_GetFopAskPrice(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		tradingClass,
		optType);
}

void SetImpliedVolatilityUpdatePeriod(int updatePeriod) {
	trdk_SetImpliedVolatilityUpdatePeriod(updatePeriod);
}

////////////////////////////////////////////////////////////////////////////////
