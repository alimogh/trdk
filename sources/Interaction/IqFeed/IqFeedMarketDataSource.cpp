/**************************************************************************
 *   Created: 2016/11/21 10:10:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IqFeedMarketDataSource.hpp"
#include "IqFeedSecurity.hpp"
#include "Core/TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::IqFeed;

namespace pt = boost::posix_time;

namespace {

	IqFeed::Settings ReadSettings(const IniSectionRef &conf) {
		const IqFeed::Settings result = {
			conf.ReadTypedKey<size_t>("history_depth"),
			conf.ReadTypedKey<size_t>("history_bar_size")
		};
		return result;
	}

}

IqFeed::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(index, context, tag)
	, m_settings(ReadSettings(conf))
	, m_stream(boost::make_unique<Stream>(*this))
	, m_history(boost::make_unique<History>(*this)) {
	//...//
}

IqFeed::MarketDataSource::~MarketDataSource() {
	m_history.reset();
	m_stream.reset();
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void IqFeed::MarketDataSource::Connect(const IniSectionRef &) {
	{
		GetLog().Debug("Connecting to the stream...");
		m_stream->Connect();
		GetLog().Info("Connected to the stream.");
	}
	{
		GetLog().Debug("Connecting to the history...");
		m_history->Connect();
		GetLog().Info("Connected to the history.");
	}
}

void IqFeed::MarketDataSource::SubscribeToSecurities() {

	GetLog().Debug(
		"Sending market data request for %1% securities...",
		m_securities.size());

	try {
	
		for (const auto &security: m_securities) {
			if (!security.second->GetRequest()) {
				security.second->SetOnline(
					security.second->GetLastMarketDataTime(),
					true);
				security.second->SetTradingSessionState(
					security.second->GetLastMarketDataTime(),
					true);
 				m_stream->SubscribeToMarketData(*security.second);
			} else {
				Assert(!security.second->IsOnline());
				m_history->SubscribeToMarketData(*security.second);
			}
		}
	
	} catch (const IqFeed::Stream::Exception &ex) {
		GetLog().Error("Failed to send market data request: \"%1%\".", ex);
		throw Error("Failed to send market data request");
	}

	GetLog().Debug("Market data request sent.");

}

void IqFeed::MarketDataSource::ResubscribeToSecurities() {

	GetLog().Debug(
		"Resending market data request for %1% securities...",
		m_securities.size());

	try {
		for (const auto &security: m_securities) {
			!security.second->IsOnline()
				?	m_history->SubscribeToMarketData(*security.second)
				:	m_stream->SubscribeToMarketData(*security.second);
		}
	} catch (const IqFeed::Stream::Exception &ex) {
		GetLog().Error("Failed to resend market data request: \"%1%\".", ex);
		throw Error("Failed to resend market data request");
	}

	GetLog().Debug("Market data request resent.");

}

trdk::Security & IqFeed::MarketDataSource::CreateNewSecurityObject(
		const Symbol &symbol) {

	const auto result = boost::make_shared<IqFeed::Security>(
		GetContext(),
		symbol,
		*this);

	switch (result->GetSymbol().GetSecurityType()) {
		case SECURITY_TYPE_FUTURES:
			{
				const auto &now = GetContext().GetCurrentTime();
				const auto &expiration
					= GetContext().GetExpirationCalendar().Find(
						symbol,
						(now + pt::hours(24)).date());
				if (!expiration) {
					boost::format error(
						"Failed to find expiration info for \"%1%\" and %2%");
					error % symbol % now;
					throw trdk::MarketDataSource::Error(error.str().c_str());
				}
				result->SetExpiration(now, *expiration);
			}
			break;
	}

	Verify(
		m_securities.emplace(result->GetSymbol().GetSymbol(), result).second);

	return *result;

}

IqFeed::Security * IqFeed::MarketDataSource::FindSecurityBySymbolString(
		const std::string &symbol) {
	const auto &it = m_securities.find(symbol);
	return it != m_securities.cend() ? &*it->second : nullptr;
}

void IqFeed::MarketDataSource::OnHistoryLoadCompleted(
		IqFeed::Security &security) {
	GetLog().Debug(
		"%1% history loaded, sending on-line market data request...",
		security);
	security.SetOnline(security.GetLastMarketDataTime(), true);
	security.SetTradingSessionState(security.GetLastMarketDataTime(), true);
	m_stream->SubscribeToMarketData(security);
	GetLog().Info(
		"%1% history loaded, on-line market data request sent.",
		security);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new IqFeed::MarketDataSource(index, context, tag, conf));
}

////////////////////////////////////////////////////////////////////////////////
