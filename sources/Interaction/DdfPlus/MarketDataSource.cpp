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

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	Stream::Credentials ReadStreamCredentials(const IniSectionRef &conf) {
		const Stream::Credentials result = {
			conf.ReadKey("server_host"),
			conf.ReadTypedKey<size_t>("server_port"),
			conf.ReadKey("login"),
			conf.ReadKey("password")
		};
		return result;
	}

	History::Settings ReadHistorySettings(const IniSectionRef &conf) {
		const History::Settings result = {
			"ds01.ddfplus.com",
			80,
			conf.ReadKey("login"),
			conf.ReadKey("password"),
			true
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
	, m_stream(
		boost::make_unique<Stream>(ReadStreamCredentials(conf), *this))
	, m_historySettings(ReadHistorySettings(conf)) {
	//...//
}

DdfPlus::MarketDataSource::~MarketDataSource() {
	m_stream.reset();
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void DdfPlus::MarketDataSource::Connect(const IniSectionRef &) {

	GetLog().Debug(
		"Connecting to stream at \"%1%:%2%\""
			" with login \"%3%\" and password...",
		m_stream->GetCredentials().host,
		m_stream->GetCredentials().port,
		m_stream->GetCredentials().login);

	m_stream->Connect();

	GetLog().Info(
		"Connected to stream at \"%1%:%2%\" with login \"%3%\" and password.",
		m_stream->GetCredentials().host,
		m_stream->GetCredentials().port,
		m_stream->GetCredentials().login);

}

void DdfPlus::MarketDataSource::SubscribeToSecurities() {

	GetLog().Debug(
		"Sending market data request for %1% securities...",
		m_securities.size());

	try {
	
		for (const auto &security: m_securities) {
			if (
					security
						.second
						->GetRequestedDataStartTime()
						.is_not_a_date_time()) {
				security.second->SetOnline();
				m_stream->SubscribeToMarketData(*security.second);
			} else {
				Assert(!security.second->IsOnline());
				CheckHistoryConnection();
				m_history->LoadHistory(*security.second);
			}
		}
	
	} catch (const DdfPlus::Stream::Exception &ex) {
		GetLog().Error("Failed to send market data request: \"%1%\".", ex);
		throw Error("Failed to send market data request");
	}

	GetLog().Debug("Market data request sent.");

}


void DdfPlus::MarketDataSource::ResubscribeToSecurities() {

	GetLog().Debug(
		"Resending market data request for %1% securities...",
		m_securities.size());

	try {
	
		for (const auto &security: m_securities) {
			if (!security.second->IsOnline()) {
				continue;
			}
			m_stream->SubscribeToMarketData(*security.second);
		}
	
	} catch (const DdfPlus::Stream::Exception &ex) {
		GetLog().Error("Failed to resend market data request: \"%1%\".", ex);
		throw Error("Failed to resend market data request");
	}

	GetLog().Debug("Market data request resent.");

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

void DdfPlus::MarketDataSource::CheckHistoryConnection() {

	if (m_history && m_history->IsConnected()) {
		return;
	}

	if (!m_history) {
		m_history.reset(new History(m_historySettings, *this));
	}

	GetLog().Debug(
		"Connecting to history at \"%1%:%2%\""
			" with login \"%3%\" and password...",
		m_history->GetSettings().host,
		m_history->GetSettings().port,
		m_history->GetSettings().login);

	m_history->Connect();

	GetLog().Info(
		"Connected to history \"%1%:%2%\" with login \"%3%\" and password.",
		m_history->GetSettings().host,
		m_history->GetSettings().port,
		m_history->GetSettings().login);

}

void DdfPlus::MarketDataSource::OnHistoryRequestCompleted(
		DdfPlus::Security &security/*,
		bool isRequestQueueEmpty*/) {

	Assert(m_history);
	Assert(!security.IsOnline());

	GetLog().Debug(
		"%1% history loaded, sending on-line market data request...",
		security);
	security.SetOnline();
	m_stream->SubscribeToMarketData(security);
	GetLog().Info(
		"%1% history loaded, on-line market data request sent.",
		security);

//	if (isRequestQueueEmpty) {
		m_history.reset();
//	}

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
