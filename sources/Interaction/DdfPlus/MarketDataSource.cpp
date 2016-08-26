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
	
	GetLog().Info(
		"Sending market data requests for %1% securities...",
		m_securities.size());

// 	boost::shared_ptr<Client> client;
// 	{
// 		const ClientLock lock(m_clientMutex);
// 		client = m_client;
// 	}
// 	if (!client) {
// 		throw ConnectError("Connection closed");
// 	}

	foreach(const auto &security, m_securities) {
		const auto &symbol = security->GetSymbol().GetSymbol();
		GetLog().Info("Sending market data request for %1%...", symbol);
		// client->SendMarketDataSubscribeRequest(symbol);
	}

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
	m_securities.push_back(result);
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
