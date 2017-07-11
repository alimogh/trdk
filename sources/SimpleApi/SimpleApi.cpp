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
#include "SimpleApiEngineServer.hpp"
#include "SimpleApiEngine.hpp"
#include "SimpleApiUtil.hpp"
#include "Core/Security.hpp"
#include "Core/EventsLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;

////////////////////////////////////////////////////////////////////////////////

namespace {

	EngineServer & GetEngineServer() {
		static EngineServer instance;
		return instance;
	}
	
	SimpleApi::Engine & GetEngine() {
		return GetEngineServer().CheckEngine(0);
	}

	EventsLog & GetLog() {
		return GetEngineServer().GetLog();
	}

}

////////////////////////////////////////////////////////////////////////////////

int32_t _stdcall trdk_InitLog(const char *filePath) {
	try {
		GetEngineServer().InitLog(filePath);
		return 1;
	} catch (const Exception &ex) {
		GetLog().Error("Failed to init log: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int32_t _stdcall trdk_EnableStdOutLog() {
	try {
		GetLog().EnableStdOut();
		return 1;
	} catch (const Exception &ex) {
		GetLog().Error("Failed to init log to StdOut: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int32_t _stdcall trdk_DestroyAllEngines() {
	try {
		GetEngineServer().DestoryAllEngines();
		return 1;
	} catch (const Exception &ex) {
		GetLog().Error("Failed to destroy all Engines: \"%1%\".", ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	const TradingSystem::Account & GetAccount(const char *account) {
		Assert(account);
		try {
			return GetEngine().GetAccount();
		} catch (const Exception &ex) {
			GetLog().Error(
				"Failed to get Account Info for \"%1%\": \"%2%\".", account, ex);
			throw;
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

}

double _stdcall GetCashBalance(const char *account) {
	try {
		return GetAccount(account).cashBalance.load();
	} catch (...) {
		AssertFailNoException();
		return 0;
	}
}
double _stdcall trdk_GetCashBalance(const char *account) {
	return GetCashBalance(account);
}

double _stdcall GetEquityWithLoanValue(const char *account) {
	try {
		return GetAccount(account).equityWithLoanValue.load();
	} catch (...) {
		AssertFailNoException();
		return 0;
	}
}
double trdk_GetEquityWithLoanValue(const char *account) {
	return GetEquityWithLoanValue(account);
}

double _stdcall GetMaintenanceMargin(const char *account) {
	try {
		return GetAccount(account).maintenanceMargin.load();
	} catch (...) {
		AssertFailNoException();
		return 0;
	}
}
double _stdcall trdk_GetMaintenanceMargin(const char *account) {
	return GetMaintenanceMargin(account);
}

double _stdcall GetExcessLiquidity(const char *account) {
	try {
		return GetAccount(account).excessLiquidity.load();
	} catch (...) {
		AssertFailNoException();
		return 0;
	}
}
double _stdcall trdk_GetExcessLiquidity(const char *account) {
	return GetExcessLiquidity(account);
}

////////////////////////////////////////////////////////////////////////////////

double _stdcall trdk_GetOptionPremium(
		const char *symbol,
		const char *currency,
		const char *exchange,
		double expirationDate,
		double strike,
		const char *right,
		double requestDate) {

	Assert(symbol);
	Assert(currency);
	Assert(exchange);
	AssertLt(0, expirationDate);
	AssertLt(0, strike);
	Assert(right);
	AssertLt(0, requestDate);
  UseUnused(requestDate);

	try {

		Security *security;
		try {
			security = &GetEngine().GetOptionSecurity(
				symbol,
				currency,
				exchange,
				static_cast<unsigned int>(expirationDate),
				strike,
				right,
        0);
		} catch (const Exception &ex) {
			GetLog().Error(
				"Failed to resolve option security ("
					"symbol: \"%1%\""
					", currency: \"%2%\""
					", exchange: \"%3%\""
					", expiration date: \"%4%\""
					", strike: \"%5%\""
					", right: \"%6%\": \"%7%\".",
				symbol,
				currency,
				exchange,
				expirationDate,
				strike,
				right,
				ex);
			return 0;
		}

		return 0;

	} catch (...) {
		GetLog().Error("Failed to get Option Premium.");
		AssertFailNoException();
	}

	return 0;
	
}

double _stdcall GetOptionPremium(
		const char *symbol,
		const char *currency,
		const char *exchange,
		double expirationDate,
		double strike,
		const char *right,
		double requestDate) {
	return trdk_GetOptionPremium(
		symbol,
		currency,
		exchange,
		expirationDate,
		strike,
		right,
		requestDate);
}

////////////////////////////////////////////////////////////////////////////////
