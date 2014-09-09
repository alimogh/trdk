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

namespace {

	const TradeSystem::Account & GetAccount(const char *account) {
		Assert(account);
		try {
			InitDebugLog();
			return theBridgeServer
				.CheckBridge(bridgeId)
				.GetAccount();
		} catch (const Exception &ex) {
			Log::Error(
				"Failed to get Account Info for \"%1%\": \"%2%\".", account, ex);
			throw;
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

}

int32_t GetNetLiquidationValueTotal(const char *account) {
	return GetAccount(account).netLiquidationValue.total;
}
int32_t GetNetLiquidationValueUsSecurities(const char *account) {
	return GetAccount(account).netLiquidationValue.usSecurities;
}
int32_t GetNetLiquidationValueUsCommodities(const char *account) {
	return GetAccount(account).netLiquidationValue.usCommodities;
}

int32_t GetEquityWithLoanValueTotal(const char *account) {
	return GetAccount(account).equityWithLoanValue.total;
}
int32_t GetEquityWithLoanValueUsSecurities(const char *account) {
	return GetAccount(account).equityWithLoanValue.usSecurities;
}
int32_t GetEquityWithLoanValueUsCommodities(const char *account) {
	return GetAccount(account).equityWithLoanValue.usCommodities;
}

int32_t GetSecuritiesGrossPositionValueTotal(const char *account) {
	return GetAccount(account).securitiesGrossPositionValue.total;
}
int32_t GetSecuritiesGrossPositionUsSecurities(const char *account) {
	return GetAccount(account).securitiesGrossPositionValue.usSecurities;
}

int32_t GetCashTotal(const char *account) {
	return GetAccount(account).cash.total;
}
int32_t GetCashUsSecurities(const char *account) {
	return GetAccount(account).cash.usSecurities;
}
int32_t GetCashUsCommodities(const char *account) {
	return GetAccount(account).cash.usCommodities;
}
	
int32_t GetCurrentInitialMarginTotal(const char *account) {
	return GetAccount(account).currentInitialMargin.total;
}
int32_t GetCurrentInitialMarginUsSecurities(const char *account) {
	return GetAccount(account).currentInitialMargin.usSecurities;
}
int32_t GetCurrentInitialMarginUsCommodities(const char *account) {
	return GetAccount(account).currentInitialMargin.usCommodities;
}
	
int32_t GetCurrentMaintenanceMarginTotal(const char *account) {
	return GetAccount(account).currentMaintenanceMargin.total;
}
int32_t GetCurrentMaintenanceMarginUsSecurities(const char *account) {
	return GetAccount(account).currentMaintenanceMargin.usSecurities;
}
int32_t GetCurrentMaintenanceMarginUsCommodities(const char *account) {
	return GetAccount(account).currentMaintenanceMargin.usCommodities;
}

int32_t GetCurrentAvailableFundsTotal(const char *account) {
	return GetAccount(account).currentAvailableFunds.total;
}
int32_t GetCurrentAvailableFundsUsSecurities(const char *account) {
	return GetAccount(account).currentAvailableFunds.usSecurities;
}
int32_t GetCurrentAvailableFundsUsCommodities(const char *account) {
	return GetAccount(account).currentAvailableFunds.usCommodities;
}

int32_t GetCurrentExcessLiquidityTotal(const char *account) {
	return GetAccount(account).currentExcessLiquidity.total;
}
int32_t GetCurrentExcessLiquidityUsSecurities(const char *account) {
	return GetAccount(account).currentExcessLiquidity.usSecurities;
}
int32_t GetCurrentExcessLiquidityUsCommodities(const char *account) {
	return GetAccount(account).currentExcessLiquidity.usCommodities;
}

////////////////////////////////////////////////////////////////////////////////
