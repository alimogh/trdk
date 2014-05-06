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

int32_t trdk_InitLog(const char *logFilePath) {
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

int32_t trdk_DestroyAllBridges() {
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

uint64_t trdk_ResolveFutOpt(
			const char *symbol,
			const char *exchange,
			const char *expirationDate,
			double strike,
			const char *right) {
	Assert(symbol);
	Assert(exchange);
	Assert(expirationDate);
	Assert(right);
	try {
		const std::string exchangeStr(exchange);
		return theBridgeServer
			.CheckBridge(bridgeId, exchangeStr)
			.ResolveFutOpt(symbol, exchangeStr, expirationDate, strike, right);
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

double trdk_GetLastPrice(uint64_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(reinterpret_cast<Bridge::SecurityId &>(securityId))
			.GetLastPrice();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Last Price across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t trdk_GetLastQty(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetLastQty();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Last Qty across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double trdk_GetAskPrice(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetAskPrice();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Ask Price across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t trdk_GetAskQty(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetAskQty();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Ask Qty across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

double trdk_GetBidPrice(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetBidPrice();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Bid Price across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t trdk_GetBidQty(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetBidQty();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Bid Qty across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t trdk_GetTradedVolume(uint32_t securityId) {
	try {
		return theBridgeServer
			.GetBridge(bridgeId)
			.GetSecurity(securityId)
			.GetTradedVolume();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Traded Volume across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
