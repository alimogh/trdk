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
#include "MqlDetail.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;
using namespace trdk::MqlApi::Detail;

////////////////////////////////////////////////////////////////////////////////

namespace {

	OrderParams GetOrderParams(const char *account) {
		OrderParams result = {};
		if (account && !Lib::IsEmpty(account)) {
			*result.account = account;
		}
		return result;
	}

}

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
	
int trdk_Buy(
			const char *symbol,
			Qty qty,
			double price,
			const char *account = nullptr) {
	try {
		auto &context = theBridge.GetContext();
		Security &security = GetSecurity(context, symbol);
		const auto priceScaled = security.ScalePrice(price);
		const auto &orderId = context.GetTradeSystem().Buy(
			security,
			qty,
			priceScaled,
			GetOrderParams(account),
			[](
						OrderId,
						TradeSystem::OrderStatus,
						Qty /*filled*/,
						Qty /*remaining*/,
						double /*avgPrice*/,
						double /*lastPrice*/) {
				//...//
			});
		return (int)orderId;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Long Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_BuyMkt(const char *symbol, Qty qty, const char *account = nullptr) {
	try {
		auto &context = theBridge.GetContext();
		Security &security = GetSecurity(context, symbol);
		const auto &orderId = context.GetTradeSystem().BuyAtMarketPrice(
			security,
			qty,
			GetOrderParams(account),
			[](
						OrderId,
						TradeSystem::OrderStatus,
						Qty /*filled*/,
						Qty /*remaining*/,
						double /*avgPrice*/,
						double /*lastPrice*/) {
				//...//
			});
		return (int)orderId;
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
			const char *symbol,
			Qty qty,
			double price,
			const char *account = nullptr) {
	try {
		auto &context = theBridge.GetContext();
		Security &security = GetSecurity(context, symbol);
		const auto priceScaled = security.ScalePrice(price);
		const auto &orderId = context.GetTradeSystem().Buy(
			security,
			qty,
			priceScaled,
			GetOrderParams(account),
			[](
						OrderId,
						TradeSystem::OrderStatus,
						Qty /*filled*/,
						Qty /*remaining*/,
						double /*avgPrice*/,
						double /*lastPrice*/) {
				//...//
			});
		return (int)orderId;
	} catch (const Exception &ex) {
		Log::Error(
			"Failed to open Short Position via MQL Bridge Server: \"%1%\".",
			ex);
	} catch (...) {
		AssertFailNoException();
	}
	return 0;
}

int trdk_SellMkt(const char *symbol, Qty qty, const char *account = nullptr) {
	try {
		auto &context = theBridge.GetContext();
		Security &security = GetSecurity(context, symbol);
		const auto &orderId = context.GetTradeSystem().SellAtMarketPrice(
			security,
			qty,
			GetOrderParams(account),
			[](
						OrderId,
						TradeSystem::OrderStatus,
						Qty /*filled*/,
						Qty /*remaining*/,
						double /*avgPrice*/,
						double /*lastPrice*/) {
				//...//
			});
		return (int)orderId;
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
		auto &context = theBridge.GetContext();
		const auto &pos
			= context
				.GetTradeSystem()
				.GetBrokerPostion(GetSymbol(context, symbol));
		return int(pos.qty);
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
