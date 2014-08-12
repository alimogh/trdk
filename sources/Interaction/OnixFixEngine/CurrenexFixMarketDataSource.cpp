/**************************************************************************
 *   Created: 2014/08/12 22:28:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CurrenexFixMarketDataSource.hpp"
#include "CurrenexSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Onyx;

namespace fix = OnixS::FIX;

CurrenexFixMarketDataSource::CurrenexFixMarketDataSource(
			const Lib::IniSectionRef &conf,
			Context::Log &log)
		: m_log(log),
		m_session("stream", conf, m_log) {
	//...//
}

CurrenexFixMarketDataSource::~CurrenexFixMarketDataSource() {
	//...//
}

void CurrenexFixMarketDataSource::Connect(const IniSectionRef &conf) {
	if (m_session.IsConnected()) {
		return;
	}
	try {
		m_session.Connect(conf, *this);
	} catch (const CurrenexFixSession::ConnectError &ex) {
		throw ConnectError(ex.what());
	} catch (const CurrenexFixSession::Error &ex) {
		throw Error(ex.what());
	}
}

CurrenexSecurity * CurrenexFixMarketDataSource::FindRequestSecurity(
			const fix::Message &requestResult) {
	const auto securityIndex
		= requestResult.getUInt64(fix::FIX42::Tags::MDReqID);
	if (securityIndex >= m_securities.size()) {
		m_log.Error(
			"Received wrong security index from FIX Server: %1%.",
			requestResult.get(fix::FIX42::Tags::MDReqID));
		return nullptr;
	}
	return &*m_securities[requestResult.getUInt64(fix::FIX42::Tags::MDReqID)];
}

const CurrenexSecurity * CurrenexFixMarketDataSource::FindRequestSecurity(
			const fix::Message &requestResult)
		const {
	return const_cast<CurrenexFixMarketDataSource *>(this)
		->FindRequestSecurity(requestResult);
}

std::string CurrenexFixMarketDataSource::GetRequestSymbolStr(
			const fix::Message &requestResult)
		const {
	const CurrenexSecurity *const security = FindRequestSecurity(requestResult);
	if (!security) {
		return std::string();
	}
	return security->GetSymbol().GetAsString();
}

void CurrenexFixMarketDataSource::SubscribeToSecurities() {

	m_log.Info(
		"Sending FIX Market Data Requests for %1% securities...",
		m_securities.size());

	for (	size_t sequrityIndex = 0;
			sequrityIndex < m_securities.size();
			++sequrityIndex) {
			
		const Symbol &symbol = m_securities[sequrityIndex]->GetSymbol();
		
		fix::Message mdRequest("V", m_session.GetFixVersion());

		mdRequest.set(
 			fix::FIX42::Tags::MDReqID,
 			sequrityIndex);
		mdRequest.setGroup(fix::FIX41::Tags::NoRelatedSym, 1)
			.at(0)
			.set(
				fix::FIX40::Tags::Symbol,
				symbol.GetSymbol() + "/" + symbol.GetCurrency());
	
		mdRequest.set(
			fix::FIX42::Tags::SubscriptionRequestType,
			fix::FIX42::Values::SubscriptionRequestType::Snapshot__plus__Updates);
		mdRequest.set(
			fix::FIX42::Tags::MarketDepth,
			fix::FIX42::Values::MarketDepth::Top_of_Book);
		mdRequest.set(
			fix::FIX42::Tags::MDUpdateType,
			fix::FIX42::Values::MDUpdateType::Incremental_Refresh);
		mdRequest.set(
			fix::FIX42::Tags::AggregatedBook,
			fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);

		auto mdEntryTypes
			= mdRequest.setGroup(fix::FIX42::Tags::NoMDEntryTypes, 2);
		mdEntryTypes[0].set(
			fix::FIX42::Tags::MDEntryType,
			fix::FIX42::Values::MDEntryType::Bid);
		mdEntryTypes[1].set(
			fix::FIX42::Tags::MDEntryType,
			fix::FIX42::Values::MDEntryType::Offer);

		m_log.Info(
			"Sending Market Data Request for %1%...",
			boost::cref(symbol));
		m_session.Get().send(&mdRequest);

	}

}

boost::shared_ptr<Security> CurrenexFixMarketDataSource::CreateSecurity(
			Context &context,
			const Symbol &symbol)
		const {
	boost::shared_ptr<CurrenexSecurity> result(
		new CurrenexSecurity(context, symbol));
	const_cast<CurrenexFixMarketDataSource *>(this)
		->m_securities.push_back(result);
	return result;
}

void CurrenexFixMarketDataSource::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void CurrenexFixMarketDataSource::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void CurrenexFixMarketDataSource::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void CurrenexFixMarketDataSource::onInboundApplicationMsg(
			fix::Message &message,
			fix::Session *session) {
			
	Assert(session == &m_session.Get());
	UseUnused(session);

	if (message.type() == "Y") {
		m_log.Error(
			"Failed to Subscribe to %1% Market Data: \"%2%\".",
			boost::make_tuple(
				GetRequestSymbolStr(message),
				message.get(fix::FIX42::Tags::MDReqRejReason)));
	} else if (message.type() == "X") {
		const Security *security = FindRequestSecurity(message);
		if (!security) {
			return;
		}
		const fix::Group &entries
			= message.getGroup(fix::FIX42::Tags::NoMDEntries);
		for (size_t i = 0; i < entries.size(); ++i) {
			
			const auto &entry = entries[i];
			const auto &entryType = entry.get(fix::FIX42::Tags::MDEntryType);
			std::string tmp;
			if (entryType == fix::FIX42::Values::MDEntryType::Bid) {
				tmp = "bid";
			} else if (entryType == fix::FIX42::Values::MDEntryType::Offer) {
				tmp = "offer";
			} else {
				tmp = "UNKNOWN";
			}
	
			m_log.Info(
				"MD!!! %1%: %2% = %3% / %4%.",
				boost::make_tuple(
					security->GetSymbol(),
					tmp,
					entry.getDouble(fix::FIX42::Tags::MDEntryPx),
					entry.getDouble(fix::FIX42::Tags::MDEntrySize)));

		}

	}
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXFIXENGINE_API
boost::shared_ptr<MarketDataSource> CreateCurrenexStream(
			const IniSectionRef &configuration,
			Context::Log &log) {
	return boost::shared_ptr<MarketDataSource>(
		new CurrenexFixMarketDataSource(configuration, log));
}

////////////////////////////////////////////////////////////////////////////////
