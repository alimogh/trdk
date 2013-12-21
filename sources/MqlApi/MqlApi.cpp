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

void trdk_InitLog(const char *logFilePath) {
	try {
		theBridge.InitLog(logFilePath);
	} catch (const Exception &ex) {
		Log::Error("Failed to init MQL Bridge Server log: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
}

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
	
int trdk_OpenLongPosition(Qty qty, double price) {
	try {
		return int(theBridge.GetStrategy().OpenLongPosition(qty, price));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open long position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_OpenShortPosition(Qty qty, double price) {
	try {
		return int(theBridge.GetStrategy().OpenShortPosition(qty, price));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open short position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

void trdk_CloseAllPositions() {
	try {
		theBridge.GetStrategy().CloseAllPositions();
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to close all positions via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
}
