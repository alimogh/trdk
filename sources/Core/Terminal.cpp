/**************************************************************************
 *   Created: 2014/10/20 01:06:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Terminal.hpp"
#include "TradeSystem.hpp"
#include "Security.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

Terminal::Terminal(const fs::path &cmdFile, TradeSystem &tradeSystem)
		: m_cmdFile(cmdFile),
		m_tradeSystem(tradeSystem),
		m_notificator(m_cmdFile, [this]() {OnCmdFileChanged();}),
		m_lastSentSeqnumber(0) {
	m_tradeSystem.GetLog().Info(
		"Starting terminal with commands file %1%...",
		m_cmdFile);
	m_notificator.Start();
	m_tradeSystem.GetLog().Info(
		"Terminal commands format:"
			" # instrument_sympol +_to_buy_or_-_to_sell"
			" price_or_0_for_market qty");
}

Terminal::~Terminal() {
	//...//
}

void Terminal::OnCmdFileChanged() {

	std::ifstream f(m_cmdFile.string().c_str());
	if (!f) {
		m_tradeSystem.GetLog().Error(
			"Failed to open terminal commands file %1%.",
			m_cmdFile);
		return;
	}

	boost::cmatch match;
	const boost::regex regex (
		"^(\\d+)\\s+([a-zA-Z1-9\\//_]+)\\s+([-\\+]+)\\s+([\\d\\.]+)\\s+(\\d+)$");
	char buffer[255] = {};
	size_t line = 0;
	size_t seqnumber = 0;
	while (!f.eof()) {
		f.getline(buffer, sizeof(buffer) - 1);
		++line;
		++seqnumber;
		if (!boost::regex_match(buffer, match, regex)) {
			m_tradeSystem.GetLog().Error(
				"Failed to parse terminal command \"%1%\" at %2%:%3%.",
				boost::make_tuple(buffer, m_cmdFile, line));
			return;
		}
		const auto cmdSeqnumber = boost::lexical_cast<size_t>(match[1]);
		if (cmdSeqnumber != seqnumber) {
			m_tradeSystem.GetLog().Error(
				"Wrong terminal command seqnumber \"%1%\" at %2%:%3%"
					", must be %4%.",
				boost::make_tuple(
					cmdSeqnumber,
					m_cmdFile,
					line,
					seqnumber));
			return;
		}
		const Symbol symbol = Symbol::ParseCash(
			Symbol::SECURITY_TYPE_CASH,
			match[2],
			"");
		const bool isBuy = match[3] == '+';
		const auto price = boost::lexical_cast<double>(match[4]);
		const auto qty = boost::lexical_cast<Qty>(match[5]);

		Security *security;
		try {
			security = &m_tradeSystem.GetContext().GetSecurity(symbol);
		} catch (const Context::UnknownSecurity &) {
			m_tradeSystem.GetLog().Error(
				"Terminal find unknown instrument in command:"
					" \"%1%\" at %2%:%3%.",
				boost::make_tuple(symbol, m_cmdFile, line));
			return;
		}

		const OrderParams orderParams;

		const auto &callback = [this](
					OrderId orderId,
					TradeSystem::OrderStatus status,
					Qty filled,
					Qty remaining,
					double avgPrice) {
			m_tradeSystem.GetLog().Info(
				"Order reply received:"
					" order ID = %1%, status = %2%, filled qty = %3%,"
					" remaining qty = %4%, avgPrice = %5%.",
				boost::make_tuple(
					orderId,
					status,
					filled,
					remaining,
					avgPrice));
		};

		if (Lib::IsZero(price)) {
			if (isBuy) {
				m_tradeSystem.BuyAtMarketPrice(
					*security,
					security->GetSymbol().GetCashCurrency(),
					qty,
					orderParams,
					callback);
			} else {
				m_tradeSystem.SellAtMarketPrice(
					*security,
					security->GetSymbol().GetCashCurrency(),
					qty,
					orderParams,
					callback);
			}
		} else {
			if (isBuy) {
				m_tradeSystem.Buy(
					*security,
					security->GetSymbol().GetCashCurrency(),
					qty,
					security->ScalePrice(price),
					orderParams,
					callback);
			} else {
				m_tradeSystem.Sell(
					*security,
					security->GetSymbol().GetCashCurrency(),
					qty,
					security->ScalePrice(price),
					orderParams,
					callback);
			}
		}
	}

}
