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
#include "MqlBridgeServer.hpp"
#include "MqlBridge.hpp"

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

void trdk_InitLogToStdOut() {
	try {
		Log::EnableEventsToStdOut();
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to init MQL Bridge Server log to StdOut: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
}

////////////////////////////////////////////////////////////////////////////////

bool trdk_CreateBridge(const char *twsHost, int twsPort, const char *account) {
	try {
		theBridge.CreateBridge(twsHost, unsigned short(twsPort), account);
		return true;
	} catch (const Exception &ex) {
		Log::Error("Failed to start MQL Bridge Server: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return false;
}

void trdk_DestoryBridge(const char *account) {
	try {
		theBridge.DestoryBridge(account);
	} catch (const Exception &ex) {
		Log::Error("Failed to stop MQL Bridge Server: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
}

void trdk_DestroyAllBridges() {
	try {
		theBridge.DestoryAllBridge();
	} catch (const Exception &ex) {
		Log::Error("Failed to stop MQL Bridge Server: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
}

////////////////////////////////////////////////////////////////////////////////
	
int trdk_Buy(
			const char *account,
			const char *symbol,
			Qty qty,
			double price) {
	try {
		return (int)theBridge.GetBridge(account).Buy(symbol, qty, price);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_BuyMkt(const char *account, const char *symbol, Qty qty) {
	try {
		return (int)theBridge.GetBridge(account).BuyMkt(symbol, qty);
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

int trdk_Sell(
			const char *account,
			const char *symbol,
			Qty qty,
			double price) {
	try {
		return (int)theBridge.GetBridge(account).Sell(symbol, qty, price);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_SellMkt(const char *account, const char *symbol, Qty qty) {
	try {
		return (int)theBridge.GetBridge(account).SellMkt(symbol, qty);
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

int trdk_GetPosition(const char *account, const char *symbol) {
	try {
		return int(theBridge.GetBridge(account).GetPosition(symbol));
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
