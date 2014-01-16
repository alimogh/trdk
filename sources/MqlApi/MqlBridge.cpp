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

Bridge::Bridge(Context &context, const std::string &account)
		: m_context(context) {
	m_orderParams.account = account;
}

OrderId Bridge::Buy(
			const std::string &symbol,
			Qty qty,
			double price) {
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	const auto &orderId = m_context.GetTradeSystem().Buy(
		security,
		qty,
		priceScaled,
		m_orderParams,
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

OrderId Bridge::BuyMkt(const std::string &symbol, Qty qty) {
	Security &security = GetSecurity(symbol);
	const auto &orderId = m_context.GetTradeSystem().BuyAtMarketPrice(
		security,
		qty,
		m_orderParams,
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

OrderId Bridge::Sell(const std::string &symbol, Qty qty, double price) {
	Security &security = GetSecurity(symbol);
	const auto priceScaled = security.ScalePrice(price);
	const auto &orderId = m_context.GetTradeSystem().Sell(
		security,
		qty,
		priceScaled,
		m_orderParams,
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

OrderId Bridge::SellMkt(const std::string &symbol, Qty qty) {
	Security &security = GetSecurity(symbol);
	const auto &orderId = m_context.GetTradeSystem().SellAtMarketPrice(
		security,
		qty,
		m_orderParams,
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

Symbol Bridge::GetSymbol(std::string symbol) const {
	boost::erase_all(symbol, ":");
	return Symbol::ParseForex(
		symbol,
		m_context.GetSettings().GetDefaultExchange());
}

Security & Bridge::GetSecurity(const std::string &symbol) const {
	return m_context.GetSecurity(GetSymbol(symbol));
}
