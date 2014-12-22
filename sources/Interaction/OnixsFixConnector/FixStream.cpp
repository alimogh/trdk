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
#include "FixStream.hpp"
#include "FixSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

FixStream::FixStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const Lib::IniSectionRef &conf)
	: MarketDataSource(index, context, tag),
	m_session(GetContext(), GetLog(), conf) {
	//...//
}

FixStream::~FixStream() {
	//...//
}

void FixStream::Connect(const IniSectionRef &conf) {
	if (m_session.IsConnected()) {
		return;
	}
	try {
		m_session.Connect(conf, *this);
	} catch (const FixSession::ConnectError &ex) {
		throw ConnectError(ex.what());
	} catch (const FixSession::Error &ex) {
		throw Error(ex.what());
	}
}

FixSecurity * FixStream::FindRequestSecurity(
			const fix::Message &requestResult) {
	const auto securityIndex
		= requestResult.getUInt64(fix::FIX42::Tags::MDReqID);
	if (securityIndex >= m_securities.size()) {
		GetLog().Error(
			"Received wrong security index: %1%.",
			requestResult.get(fix::FIX42::Tags::MDReqID));
		return nullptr;
	}
	const auto securityId = requestResult.getUInt64(fix::FIX42::Tags::MDReqID);
	return &*m_securities[size_t(securityId)];
}

const FixSecurity * FixStream::FindRequestSecurity(
			const fix::Message &requestResult)
		const {
	return const_cast<FixStream *>(this)
		->FindRequestSecurity(requestResult);
}

std::string FixStream::GetRequestSymbolStr(
			const fix::Message &requestResult)
		const {
	const FixSecurity *const security = FindRequestSecurity(requestResult);
	if (!security) {
		return std::string();
	}
	return security->GetSymbol().GetAsString();
}

void FixStream::SubscribeToSecurities() {

	GetLog().Info(
		"Sending Market Data Requests for %1% securities...",
		m_securities.size());

	for (
			fix::UInt32 sequrityIndex = 0;
			sequrityIndex < m_securities.size();
			++sequrityIndex) {
			
		const Symbol &symbol = m_securities[sequrityIndex]->GetSymbol();
		
		fix::Message mdRequest("V", m_session.GetFixVersion());

		mdRequest.set(
 			fix::FIX42::Tags::MDReqID,
 			sequrityIndex);
		mdRequest.setGroup(fix::FIX41::Tags::NoRelatedSym, 1)
			.at(0)
			.set(fix::FIX40::Tags::Symbol, symbol.GetSymbol());
	
		mdRequest.set(
			fix::FIX42::Tags::SubscriptionRequestType,
			fix::FIX42::Values::SubscriptionRequestType::Snapshot__plus__Updates);
		mdRequest.set(
			fix::FIX42::Tags::MarketDepth,
			fix::FIX42::Values::MarketDepth::Full_Book);
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

		GetLog().Info(
			"Sending Market Data Request for %1%...",
			boost::cref(symbol));
		m_session.Get().send(&mdRequest);

	}

}

Security & FixStream::CreateSecurity(const Symbol &symbol) {
	boost::shared_ptr<FixSecurity> result(
		new FixSecurity(GetContext(), symbol, *this));
	const_cast<FixStream *>(this)
		->m_securities.push_back(result);
	return *result;
}

void FixStream::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {

	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);

	if (
			prevState == fix::SessionState::LogoutInProgress
			&& newState == fix::SessionState::Disconnected) {
		OnLogout();
	} else if (
			prevState == fix::SessionState::Active
			&& newState == fix::SessionState::Reconnecting) {
		OnReconnecting();
	}
	
	if (newState == fix::SessionState::Disconnected) {
		m_session.Reconnect();
	}

}

void FixStream::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void FixStream::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void FixStream::onInboundApplicationMsg(
			fix::Message &message,
			fix::Session *session) {

	const auto &timeMeasurement = GetContext().StartStrategyTimeMeasurement();
			
	Assert(session == &m_session.Get());
	UseUnused(session);

	if (message.type() == "Y") {
		
		GetLog().Error(
			"Failed to subscribe to %1% market data: \"%2%\".",
			GetRequestSymbolStr(message),
			message.get(fix::FIX42::Tags::MDReqRejReason));

	} else if (message.type() == "X") {

		FixSecurity *const security = FindRequestSecurity(message);
		if (!security) {
			return;
		}
		
		FixSecurity::BookUpdateOperation book = security->StartBookUpdate();

		const fix::Group &entries
			= message.getGroup(fix::FIX42::Tags::NoMDEntries);
		AssertLt(0, entries.size());
		for (size_t i = 0; i < entries.size(); ++i) {
			
			const auto &entry = entries[i];
			const auto &entryType = entry.get(fix::FIX42::Tags::MDEntryType);
			Assert(
				entryType == fix::FIX42::Values::MDEntryType::Bid
					|| entryType == fix::FIX42::Values::MDEntryType::Offer);
			FixSecurity::BookUpdateOperation::Side &side
				= entryType == fix::FIX42::Values::MDEntryType::Bid
						?	book.GetBids()
						:	book.GetOffers();

			const auto price = entry.getDouble(fix::FIX42::Tags::MDEntryPx);
			const auto &action = entry.get(fix::FIX42::Tags::MDUpdateAction);
			if (action == fix::FIX42::Values::MDUpdateAction::Change) {
				side.Update(price, ParseMdEntrySize(entry));
			} else if (action == fix::FIX42::Values::MDUpdateAction::New) {
				side.Add(price, ParseMdEntrySize(entry));
			} else  if (action == fix::FIX42::Values::MDUpdateAction::Delete) {
				side.Delete(price);
			} else {
				AssertFail("Unknown entry type.");
				continue;
			}

		}

		book.Commit(timeMeasurement);

	}
}

Qty FixStream::ParseMdEntrySize(const fix::GroupInstance &entry) const {
	return entry.getInt32(fix::FIX42::Tags::MDEntrySize);
}
