/**************************************************************************
 *   Created: 2016/08/24 09:52:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace {

	Connection::Credentials ReadCredentials(const IniSectionRef &conf) {
		const Connection::Credentials result = {
			conf.ReadKey("server_host"),
			conf.ReadTypedKey<size_t>("server_port"),
			conf.ReadKey("login"),
			conf.ReadKey("password")
		};
		return result;
	}

}

DdfPlus::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(index, context, tag)
	, m_connection(ReadCredentials(conf), *this) {
	//...//
}

DdfPlus::MarketDataSource::~MarketDataSource() {
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void DdfPlus::MarketDataSource::Connect(const IniSectionRef &) {

	GetLog().Debug(
		"Connecting to \"%1%:%2%\" with login \"%3%\" and password...",
		m_connection.GetCredentials().host,
		m_connection.GetCredentials().port,
		m_connection.GetCredentials().login);

	m_connection.Connect();

	GetLog().Info(
		"Connected to \"%1%:%2%\" with login \"%3%\" and password.",
		m_connection.GetCredentials().host,
		m_connection.GetCredentials().port,
		m_connection.GetCredentials().login);

}

void DdfPlus::MarketDataSource::SubscribeToSecurities() {
	
	{
		std::vector<std::string> symbols;
		for (const auto &security: m_securities) {
			boost::format symbol("%1% (%2%)");
			symbol % *security % security->GenerateDdfPlusCode();
			symbols.emplace_back(symbol.str());
		}
		GetLog().Info(
			"Sending market data request for %1% securities: %2%...",
			m_securities.size(),
			boost::join(symbols, ", "));
	}

	try {
		m_connection.SubscribeToMarketData(m_securities);
	} catch (const DdfPlus::Connection::Exception &ex) {
		GetLog().Error(
			"Failed to send market data request: \"%1%\".",
			ex);
		throw Error("Failed to send market data request");
	}
	GetLog().Debug("Market data request sent.");

}

trdk::MarketDataSource & DdfPlus::MarketDataSource::GetSource() {
	return *this;
}
const trdk::MarketDataSource & DdfPlus::MarketDataSource::GetSource() const {
	return *this;
}

trdk::Security & DdfPlus::MarketDataSource::CreateNewSecurityObject(
		const Symbol &symbol) {
	
	const auto result
		= boost::make_shared<DdfPlus::Security>(GetContext(), symbol, *this);

	switch (result->GetSymbol().GetSecurityType()) {
		case SECURITY_TYPE_FUTURES:
		{
			const auto &now = GetContext().GetCurrentTime();
			const auto &expiration
				= GetContext().GetExpirationCalendar().Find(symbol, now);
			if (!expiration) {
				boost::format error(
					"Failed to find expiration info for \"%1%\" and %2%");
				error % symbol % now;
				throw trdk::MarketDataSource::Error(error.str().c_str());
			}
			result->SetExpiration(*expiration);
			GetLog().Info(
				"Current expiration date for \"%1%\": %2% (%3%).",
				symbol,
				result->GetExpiration().expirationDate,
				result->GenerateDdfPlusCode());
		}
		break;
	}

	m_securities.emplace_back(result);

	return *result;

}

TRDK_INTERACTION_DDFPLUS_API boost::shared_ptr<trdk::MarketDataSource>
CreateMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new DdfPlus::MarketDataSource(
			index,
			context,
			tag,
			conf));
}
