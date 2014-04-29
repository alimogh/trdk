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
#include "CApiBridgeServer.hpp"
#include "CApiBridge.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::CApi;

namespace {
	BridgeServer theBridgeServer;
}

////////////////////////////////////////////////////////////////////////////////

int trdk_InitLog(const char *logFilePath) {
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

int trdk_InitLogToStdOut() {
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

int trdk_CreateIbTwsBridge(
		const char *twsHost,
		unsigned short twsPort,
		const char *account,
		const char *defaultExchange,
		const char *expirationDate,
		double strike) {
	try {
		theBridgeServer.CreateIbTwsBridge(
			twsHost,
			twsPort,
			account,
			defaultExchange,
			expirationDate,
			strike);
		Sleep(3000); // @todo Only for debug, remove.
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to create Bridge to IB TWS: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_DestroyBridge(const char *account) {
	try {
		theBridgeServer.DestoryBridge(account);
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to destroy Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_DestroyAllBridges() {
	try {
		theBridgeServer.DestoryAllBridge();
		return 1;
	} catch (const Exception &ex) {
		Log::Error("Failed to destroy all  Bridges: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

double trdk_GetCashBalance(const char *account) {
	try {
		return theBridgeServer.GetBridge(account).GetCashBalance();
	} catch (const Exception &ex) {
		Log::Error("Failed to get Cash Balance across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_GetPosition(const char *account, const char *symbol) {
	try {
		return int(theBridgeServer.GetBridge(account).GetPosition(symbol));
	} catch (const Exception &ex) {
		Log::Error("Failed to get Position across Bridge: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////
