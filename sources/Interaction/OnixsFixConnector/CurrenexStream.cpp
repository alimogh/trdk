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
#include "CurrenexStream.hpp"
#include "CurrenexSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

CurrenexStream::CurrenexStream(
			const Lib::IniSectionRef &conf,
			Context::Log &log)
		: m_log(log),
		m_session("stream", conf, m_log) {
	//...//
}

CurrenexStream::~CurrenexStream() {
	//...//
}

void CurrenexStream::Connect(const IniSectionRef &conf) {
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

CurrenexSecurity * CurrenexStream::FindRequestSecurity(
			const fix::Message &requestResult) {
	const auto securityIndex
		= requestResult.getUInt64(fix::FIX42::Tags::MDReqID);
	if (securityIndex >= m_securities.size()) {
		m_log.Error(
			"Received wrong security index from FIX Server: %1%.",
			requestResult.get(fix::FIX42::Tags::MDReqID));
		return nullptr;
	}
	const auto securityId = requestResult.getUInt64(fix::FIX42::Tags::MDReqID);
	return &*m_securities[size_t(securityId)];
}

const CurrenexSecurity * CurrenexStream::FindRequestSecurity(
			const fix::Message &requestResult)
		const {
	return const_cast<CurrenexStream *>(this)
		->FindRequestSecurity(requestResult);
}

std::string CurrenexStream::GetRequestSymbolStr(
			const fix::Message &requestResult)
		const {
	const CurrenexSecurity *const security = FindRequestSecurity(requestResult);
	if (!security) {
		return std::string();
	}
	return security->GetSymbol().GetAsString();
}

void CurrenexStream::SubscribeToSecurities() {

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

boost::shared_ptr<Security> CurrenexStream::CreateSecurity(
			Context &context,
			const Symbol &symbol)
		const {
	boost::shared_ptr<CurrenexSecurity> result(
		new CurrenexSecurity(context, symbol));
	const_cast<CurrenexStream *>(this)
		->m_securities.push_back(result);
	return result;
}

void CurrenexStream::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void CurrenexStream::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void CurrenexStream::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void CurrenexStream::onInboundApplicationMsg(
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
	
		CurrenexSecurity *const security = FindRequestSecurity(message);
		if (!security) {
			return;
		}
		
		const fix::Group &entries
			= message.getGroup(fix::FIX42::Tags::NoMDEntries);
		for (size_t i = 0; i < entries.size(); ++i) {
			
			const auto &entry = entries[i];

			const auto price = entry.getDouble(fix::FIX42::Tags::MDEntryPx);
			const Qty qty = entry.getInt32(fix::FIX42::Tags::MDEntrySize);
			
			const auto &entryType = entry.get(fix::FIX42::Tags::MDEntryType);
			if (entryType == fix::FIX42::Values::MDEntryType::Bid) {
				security->SetBid(price, qty);
			} else if (entryType == fix::FIX42::Values::MDEntryType::Offer) {
				security->SetOffer(price, qty);
			} else {
				AssertFail("Unknown entry type.");
				continue;
			}

		}

	}
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource> CreateCurrenexStream(
			const IniSectionRef &configuration,
			Context::Log &log) {
	return boost::shared_ptr<MarketDataSource>(
		new CurrenexStream(configuration, log));
}

////////////////////////////////////////////////////////////////////////////////
