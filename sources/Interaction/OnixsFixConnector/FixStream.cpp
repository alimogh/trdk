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
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;
namespace pt = boost::posix_time;

FixStream::FixStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const Lib::IniSectionRef &conf)
	: MarketDataSource(index, context, tag)
	, m_session(GetContext(), GetLog(), conf)
	, m_isSubscribed(false) {
	//...//	
}

FixStream::~FixStream() {
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
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

	const StateLock lock(m_stateMutex);
	if (m_state != fix::SessionState::Active) {
		m_isSubscribed = true;
		return;
	}

	GetLog().Info(
		"Sending Market Data Requests for %1% securities...",
		m_securities.size());

	for (
			fix::UInt32 sequrityIndex = 0;
			sequrityIndex < m_securities.size();
			++sequrityIndex) {
			
		const Security &security = *m_securities[sequrityIndex];
		const Symbol &symbol = security.GetSymbol();
		
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
			fix::FIX42::Tags::MDUpdateType,
			fix::FIX42::Values::MDUpdateType::Incremental_Refresh);

		SetupBookRequest(mdRequest, security);

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

	m_isSubscribed = true;

}

Security & FixStream::CreateNewSecurityObject(const Symbol &symbol) {
	const auto &result
		= boost::make_shared<FixSecurity>(GetContext(), symbol, *this);
	const_cast<FixStream *>(this)->m_securities.push_back(result);
	return *result;
}

void FixStream::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {

	{
		const StateLock lock(m_stateMutex);
		m_state = newState;
	}

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
	
	if (
			newState == fix::SessionState::Disconnected
			|| newState == fix::SessionState::Reconnecting) {
		foreach (const auto &security, m_securities) {
			security->ClearBook();
		}
	}

	if (newState == fix::SessionState::Disconnected) {

		m_session.Reconnect();

	} else if (
			prevState != fix::SessionState::Active
			&& newState == fix::SessionState::Active
			&& m_isSubscribed) {
		SubscribeToSecurities();
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
	const auto &now = GetContext().GetCurrentTime();
			
	Assert(session == &m_session.Get());
	UseUnused(session);

	FixSecurity *security = nullptr;

	try {

		if (message.type() == "Y") {
		
			GetLog().Error(
				"Failed to subscribe to %1% market data: \"%2%\".",
				GetRequestSymbolStr(message),
				message.get(fix::FIX42::Tags::MDReqRejReason));
			return;

		}

		if (message.type() == "X") {

			security = FindRequestSecurity(message);
			if (!security) {
				return;
			}
		
			const fix::Group &entries
				= message.getGroup(fix::FIX42::Tags::NoMDEntries);
			AssertLt(0, entries.size());
			for (size_t i = 0; i < entries.size(); ++i) {

				const auto &entry = entries[i];
				auto entryId = entry.getInt64(fix::FIX42::Tags::MDEntryID);
				const auto &action = entry.get(
					fix::FIX42::Tags::MDUpdateAction);

				if (action == fix::FIX42::Values::MDUpdateAction::Change) {

					const auto &qty = ParseMdEntrySize(entry);
					if (qty == 0) {
						GetLog().Warn(
							"Price level with zero-qty received for %1%: \"%2%\".",
							*security,
							message);
						continue;
					}

					const auto price = entry.getDouble(
						fix::FIX42::Tags::MDEntryPx);
					bool isHandled = false;

					if (entry.contain(fix::FIX42::Tags::MDEntryRefID)) {
						const auto entryRefId
							= entry.getInt64(fix::FIX42::Tags::MDEntryRefID);
						if (entryRefId != 1) {
							isHandled = true;
							security->OnEntryReplace(
								entryRefId,
								entryId,
								now,
								entry.get(fix::FIX42::Tags::MDEntryType)
									== fix::FIX42::Values::MDEntryType::Bid,
								entry.getDouble(fix::FIX42::Tags::MDEntryPx),
								ParseMdEntrySize(entry));
						}
					}
				
					if (!isHandled) {
						security->OnEntryUpdate(
							entryId,
							now,
							entry.get(fix::FIX42::Tags::MDEntryType)
								== fix::FIX42::Values::MDEntryType::Bid,
							price,
							qty);
					}

				} else if (action == fix::FIX42::Values::MDUpdateAction::New) {

					const auto &qty = ParseMdEntrySize(entry);
					if (qty == 0) {
						GetLog().Warn(
							"Price level with zero-qty received for %1%: \"%2%\".",
							*security,
							message);
						continue;
					}

					security->OnNewEntry(
						entryId,
						now,
						entry.get(fix::FIX42::Tags::MDEntryType)
							== fix::FIX42::Values::MDEntryType::Bid,
						entry.getDouble(fix::FIX42::Tags::MDEntryPx),
						qty);

				} else  if (
						action == fix::FIX42::Values::MDUpdateAction::Delete) {

					security->OnEntryDelete(entryId);

				} else {

					AssertFail("Unknown entry type.");
					continue;

				}

			}

		} else if (message.type() == "W") {

			security = FindRequestSecurity(message);
			if (!security) {
				return;
			}

			OnMarketDataSnapshot(message, now, *security);

		} else {
		
			return;
	
		}

	} catch (const std::exception &ex) {
		GetLog().Error(
			"Fatal error"
				" in the processing of incoming application messages"
				": \"%1%\".",
			ex.what());
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}

	security->Flush(now, timeMeasurement);

}

void FixStream::OnMarketDataSnapshot(
		const fix::Message &message,
		const pt::ptime &time,
		FixSecurity &security) {

	const fix::Group &entries = message.getGroup(fix::FIX42::Tags::NoMDEntries);
	for (size_t i = 0; i < entries.size(); ++i) {

		const auto &entry = entries[i];

		if (
				entry.get(fix::FIX42::Tags::QuoteCondition)
					!= fix::FIX42::Values::QuoteCondition::Open_Active) {
			GetTradingLog().Write(
				"Inactive stream for %1%.",
				[&security](TradingRecord &record)  {
					record % security;
				});
		}


		const double price = entry.getDouble(fix::FIX42::Tags::MDEntryPx);
		if (IsZero(price)) {
			GetTradingLog().Write(
				"Price level with zero-price received for %1%.",
				[&security](TradingRecord &record)  {
					record % security;
				});
			continue;
		}

		const auto &qty = ParseMdEntrySize(entry);
		if (qty == 0) {
			GetTradingLog().Write(
				"Price level with zero-qty received for %1%.",
				[&security](TradingRecord &record)  {
					record % security;
				});
			continue;
		}

		security.OnSnapshotEntry(
			time,
			entry.get(fix::FIX42::Tags::MDEntryType)
				== fix::FIX42::Values::MDEntryType::Bid,
			price,
			qty);

	}

}

Qty FixStream::ParseMdEntrySize(const fix::GroupInstance &entry) const {
	return Qty(entry.getDouble(fix::FIX42::Tags::MDEntrySize));
}

Qty FixStream::ParseMdEntrySize(const fix::Message &message) const {
	return Qty(message.getDouble(fix::FIX42::Tags::MDEntrySize));
}
