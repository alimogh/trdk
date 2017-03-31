/**************************************************************************
 *   Created: 2014/04/29 23:28:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SimpleApiEngine.hpp"
#include "Services/BarService.hpp"
#include "Engine/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::SimpleApi;
using namespace trdk::Services;


SimpleApi::Engine::Engine(
		const boost::shared_ptr<trdk::Engine::Context> &context,
		const TradingMode &tradingMode,
		size_t tradingSystemIndex)
	: m_context(context)
	, m_tradingMode(tradingMode)
	, m_tradingSystemIndex(tradingSystemIndex) {
	//...//
}

const TradingSystem::Account & SimpleApi::Engine::GetAccount() const {
	return m_context
		->GetTradingSystem(m_tradingSystemIndex, m_tradingMode)
		.GetAccount();
}

Security & SimpleApi::Engine::GetOptionSecurity(
		const std::string &symbolName,
		const std::string &currency,
		const std::string &exchange,
		unsigned int expirationDate,
		double strike,
		const std::string &right,
		const unsigned int dataStartDate) {

	Symbol symbol;
	symbol.SetSecurityType(SECURITY_TYPE_OPTIONS);
	symbol.SetSymbol(symbolName);
	symbol.SetCurrency(ConvertCurrencyFromIso(currency));
	symbol.SetExchange(exchange);
	symbol.SetStrike(strike);
	symbol.SetRight(right);

	const ContractExpiration expiration(
		ConvertToDate(static_cast<unsigned int>(expirationDate)));

	auto &source = m_context->GetMarketDataSource(0);

	Security *security = source.FindSecurity(symbol, expiration);
	if (!security) {
		security = &source.GetSecurity(symbol, expiration);
		{
			Security::Request request;
			request.RequestTime(
				ConvertToTime(dataStartDate, 0) - pt::hours(25));
			security->SetRequest(request);
		}
		source.SubscribeToSecurities();
	}

	return *security;

}
