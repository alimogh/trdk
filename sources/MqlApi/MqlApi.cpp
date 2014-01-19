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

namespace {

	bool IsPut(const std::string &option) {
		if (boost::iequals(option, "put")) {
			return true;
		} else if (boost::iequals(option, "call")) {
			return false;
		} else {
			Log::Error(
				"MQL Bridge Server failed to resolve security (Put or Call):"
					" \"%1%\".",
				option);
			throw Exception(
				"MQL Bridge Server failed to resolve security (Put or Call)");
		}
	}

}

////////////////////////////////////////////////////////////////////////////////

void trdk_InitLog(const char *logFilePath) {
	try {
		theBridge.InitLog(logFilePath);
	} catch (const Exception &ex) {
		Log::Error("Failed to init MQL Bridge Server log: \"%1%\".", ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void trdk_InitLogToStdOut() {
	try {
		Log::EnableEventsToStdOut();
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to init MQL Bridge Server log to StdOut: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool trdk_CreateBridge(
		const char *twsHost,
		int twsPort,
		const char *account,
		const char *defaultExchange,
		const char *expirationDate,
		double strike) {
	try {
		theBridge.CreateBridge(
			twsHost,
			unsigned short(twsPort),
			account,
			defaultExchange,
			expirationDate,
			strike);
		Sleep(10000); // @todo Only for debug, remove.
		return true;
	} catch (const Exception &ex) {
		Log::Error("Failed to start MQL Bridge Server: \"%1%\".", ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void trdk_DestoryBridge(const char *account) {
	try {
		theBridge.DestoryBridge(account);
	} catch (const Exception &ex) {
		Log::Error("Failed to stop MQL Bridge Server: \"%1%\".", ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void trdk_DestroyAllBridges() {
	try {
		theBridge.DestoryAllBridge();
	} catch (const Exception &ex) {
		Log::Error("Failed to stop MQL Bridge Server: \"%1%\".", ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

////////////////////////////////////////////////////////////////////////////////
	
int trdk_Buy(
			const char *account,
			const char *symbol,
			const char *option,
			Qty qty,
			double price) {
	try {
		return (int)theBridge.GetBridge(account).Buy(
			symbol,
			IsPut(option),
			qty,
			price);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

int trdk_BuyMkt(
			const char *account,
			const char *symbol,
			const char *option,
			Qty qty) {
	try {
		return (int)theBridge.GetBridge(account).BuyMkt(
			symbol,
			IsPut(option),
			qty);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position at market price"
				" via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

////////////////////////////////////////////////////////////////////////////////

int trdk_Sell(
			const char *account,
			const char *symbol,
			const char *option,
			Qty qty,
			double price) {
	try {
		return (int)theBridge.GetBridge(account).Sell(
			symbol,
			IsPut(option),
			qty,
			price);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

int trdk_SellMkt(
			const char *account,
			const char *symbol,
			const char *option,
			Qty qty) {
	try {
		return (int)theBridge.GetBridge(account).SellMkt(
			symbol,
			IsPut(option),
			qty);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position at market price"
				" via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

////////////////////////////////////////////////////////////////////////////////

double trdk_GetCashBalance(const char *account) {
	try {
		return theBridge.GetBridge(account).GetCashBalance();
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Cash Balance via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

int trdk_GetPosition(const char *account, const char *symbol) {
	try {
		return int(theBridge.GetBridge(account).GetPosition(symbol));
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Position via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

int trdk_GetAllPositions(
			const char *account,
			MqlTypes::String *symbolsResult,
			size_t symbolsResultBufferSize,
			int32_t *qtyResult,
			size_t qtyResultBufferSize) {
	try {
		const auto result = theBridge.GetBridge(account).GetAllPositions(
			symbolsResult,
			symbolsResultBufferSize,
			qtyResult,
			qtyResultBufferSize);
		return int(result);
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to get Position via MQL Bridge Server: \"%1%\".",
			ex);
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	
}

////////////////////////////////////////////////////////////////////////////////
