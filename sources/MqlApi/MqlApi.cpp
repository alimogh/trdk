/**************************************************************************
 *   Created: 2013/12/16 21:40:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MqlBridgeStrategy.hpp"
#include "MqlBridgeServer.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;

////////////////////////////////////////////////////////////////////////////////

void trdk_InitLog(const char *logFilePath) {
	try {
		theBridge.InitLog(logFilePath);
	} catch (const Exception &ex) {
		Log::Error("Failed to init MQL Bridge Server log: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
}

////////////////////////////////////////////////////////////////////////////////

int trdk_CreateBridge() {
	try {
		if (theBridge.IsActive()) {
			Log::Warn("MQL Bridge Server is already started.");
			return 1;
		}
		theBridge.Run();
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to start MQL Bridge Server: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

void trdk_DeleteBridge() {
	try {
		if (!theBridge.IsActive()) {
			Log::Warn("MQL Bridge Server doesn't exist.");
			return;
		}
		theBridge.Stop();
	} catch (const Exception &ex) {
		Log::Error("Failed to stop MQL Bridge Server: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
}

////////////////////////////////////////////////////////////////////////////////
	
int trdk_OpenLongPosition(const char *symbol, Qty qty, double price) {
	try {
		return int(theBridge.GetStrategy().OpenLongPosition(
			symbol,
			qty,
			price));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_OpenLongPositionMkt(const char *symbol, Qty qty) {
	try {
		return int(theBridge.GetStrategy().OpenLongPositionByMarketPrice(
			symbol,
			qty));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position at market price"
				" via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int trdk_OpenShortPosition(const char *symbol, Qty qty, double price) {
	try {
		return int(theBridge.GetStrategy().OpenShortPosition(
			symbol,
			qty,
			price));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_OpenShortPositionMkt(const char *symbol, Qty qty) {
	try {
		return int(theBridge.GetStrategy().OpenShortPositionByMarketPrice(
			symbol,
			qty));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position at market price"
				" via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int trdk_GetPosition(const char *symbol) {
	try {
		return int(theBridge.GetStrategy().GetPositionQty(symbol));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
