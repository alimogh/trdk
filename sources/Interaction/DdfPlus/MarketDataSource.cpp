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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

DdfPlus::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(index, context, tag)
	, m_connection(
		boost::make_unique<Connection>(ReadCredentials(conf), *this)) {
	//...//
}

DdfPlus::MarketDataSource::~MarketDataSource() {
	m_connection.reset();
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void DdfPlus::MarketDataSource::Connect(const IniSectionRef &) {

	GetLog().Debug(
		"Connecting to \"%1%:%2%\" with login \"%3%\" and password...",
		m_connection->GetCredentials().host,
		m_connection->GetCredentials().port,
		m_connection->GetCredentials().login);

	m_connection->Connect();

	GetLog().Info(
		"Connected to \"%1%:%2%\" with login \"%3%\" and password.",
		m_connection->GetCredentials().host,
		m_connection->GetCredentials().port,
		m_connection->GetCredentials().login);

}

void DdfPlus::MarketDataSource::SubscribeToSecurities() {
	GetLog().Debug(
		"Sending market data request for %1% securities...",
		m_securities.size());
	try {
		for (const auto &security : m_securities) {
			m_connection->SubscribeToMarketData(*security.second);
		}
	} catch (const DdfPlus::Connection::Exception &ex) {
		GetLog().Error("Failed to send market data request: \"%1%\".", ex);
		throw Error("Failed to send market data request");
	}
	GetLog().Debug("Market data request sent.");
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
				result->GetExpiration().GetDate(),
				result->GenerateDdfPlusCode());
		}
		break;
	}

	Assert(m_securities.count(result->GenerateDdfPlusCode()) == 0);
	m_securities.emplace(std::make_pair(result->GenerateDdfPlusCode(), result));

	return *result;

}

DdfPlus::Security * DdfPlus::MarketDataSource::FindSecurity(
			const std::string &ddfPlusCodeSymbolCode) {
	const auto &result = m_securities.find(ddfPlusCodeSymbolCode);
	return result != m_securities.cend()
		? &*result->second
		: nullptr;
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
