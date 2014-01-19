/**************************************************************************
 *   Created: 2014/01/16 21:40:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MqlBridge.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::MqlApi;

Bridge::Bridge(
			Context &context,
			const std::string &account,
			const std::string &expirationDate,
			double strike)
		: m_context(context),
		m_expirationDate(expirationDate),
		m_strike(strike) {
	m_orderParams.account = account;
}

OrderId Bridge::Buy(
			const std::string &symbol,
			bool isPut,
			Qty qty,
			double price) {
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	const auto &orderId = m_context.GetTradeSystem().Buy(
		security,
		qty,
		priceScaled,
		GetOrderParams(isPut),
		[](
					OrderId,
					TradeSystem::OrderStatus,
					Qty /*filled*/,
					Qty /*remaining*/,
					double /*avgPrice*/,
					double /*lastPrice*/) {
			//...//
		});
	return orderId;
}

OrderId Bridge::BuyMkt(const std::string &symbol, bool isPut, Qty qty) {
	Security &security = GetSecurity(symbol);
	const auto &orderId = m_context.GetTradeSystem().BuyAtMarketPrice(
		security,
		qty,
		GetOrderParams(isPut),
		[](
					OrderId,
					TradeSystem::OrderStatus,
					Qty /*filled*/,
					Qty /*remaining*/,
					double /*avgPrice*/,
					double /*lastPrice*/) {
			//...//
		});
	return orderId;
}

OrderId Bridge::Sell(
			const std::string &symbol,
			bool isPut,
			Qty qty,
			double price) {
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	const auto &orderId = m_context.GetTradeSystem().Sell(
		security,
		qty,
		priceScaled,
		GetOrderParams(isPut),
		[](
					OrderId,
					TradeSystem::OrderStatus,
					Qty /*filled*/,
					Qty /*remaining*/,
					double /*avgPrice*/,
					double /*lastPrice*/) {
			//...//
		});
	return orderId;
}

OrderId Bridge::SellMkt(const std::string &symbol, bool isPut, Qty qty) {
	Security &security = GetSecurity(symbol);
	const auto &orderId = m_context.GetTradeSystem().SellAtMarketPrice(
		security,
		qty,
		GetOrderParams(isPut),
		[](
					OrderId,
					TradeSystem::OrderStatus,
					Qty /*filled*/,
					Qty /*remaining*/,
					double /*avgPrice*/,
					double /*lastPrice*/) {
			//...//
		});
	return orderId;
}

Qty Bridge::GetPosition(const std::string &symbol) const {
	const auto &pos
		= m_context
			.GetTradeSystem()
			.GetBrokerPostion(*m_orderParams.account, GetSymbol(symbol));
	return pos.qty;
}

size_t Bridge::GetAllPositions(
			MqlTypes::String *symbolsResult,
			size_t symbolsResultBufferSize,
			int32_t *qtyResult,
			size_t qtyResultBufferSize)
		const {
	size_t count = 0;
	m_context.GetTradeSystem().ForEachBrokerPostion(
		*m_orderParams.account, 
		[&](const TradeSystem::Position &position) -> bool {
			if (	position.symbol.GetSecurityType()
						!= Symbol::SECURITY_TYPE_FUTURE_OPTION) {
				return true;
			}
			AssertEq(*m_orderParams.account, position.account);
			// warning! under lock!
			if (count < symbolsResultBufferSize) {
				symbolsResult[count]
					= position.symbol.GetSymbol()
						+ position.symbol.GetCurrency();
			}
			if (count < qtyResultBufferSize) {
				qtyResult[count] = position.qty;
			}
			++count;
			return true;
		});
	return count;
}

double Bridge::GetCashBalance() const {
	return m_context.GetTradeSystem().GetAccount().cashBalance;
}

Symbol Bridge::GetSymbol(std::string symbol) const {
	boost::erase_all(symbol, ":");
	return Symbol::ParseCashFutureOption(
 		symbol,
 		m_expirationDate,
 		m_strike,
 		m_context.GetSettings().GetDefaultExchange());
}

Security & Bridge::GetSecurity(const std::string &symbol) const {
	return m_context.GetSecurity(GetSymbol(symbol));
}

OrderParams Bridge::GetOrderParams(bool isPut) const {
	auto result = m_orderParams;
	result.isPut = isPut;
	return result;
}
