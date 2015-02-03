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
			fix::FIX42::Tags::MDUpdateType,
			fix::FIX42::Values::MDUpdateType::Incremental_Refresh);

		SetupBookRequest(mdRequest);

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
	const auto &now = GetContext().GetCurrentTime();
			
	Assert(session == &m_session.Get());
	UseUnused(session);

	if (message.type() == "Y") {
		
		GetLog().Error(
			"Failed to subscribe to %1% market data: \"%2%\".",
			GetRequestSymbolStr(message),
			message.get(fix::FIX42::Tags::MDReqRejReason));
		return;

	}

	FixSecurity *const security = FindRequestSecurity(message);
	if (!security) {
		return;
	}
	
	if (message.type() == "X") {
		
		const fix::Group &entries
			= message.getGroup(fix::FIX42::Tags::NoMDEntries);
		AssertLt(0, entries.size());
		for (size_t i = 0; i < entries.size(); ++i) {

			const auto &entry = entries[i];
			auto entryId = entry.getInt64(fix::FIX42::Tags::MDEntryID);
			const auto &action = entry.get(fix::FIX42::Tags::MDUpdateAction);

			if (action == fix::FIX42::Values::MDUpdateAction::Change) {

				const auto &qty = ParseMdEntrySize(entry);
				if (IsZero(qty)) {
					GetLog().Error(
						"Price level with zero-qty received for %1%: \"%2%\".",
						*security,
						message);
					continue;
				}

				const auto price = entry.getDouble(fix::FIX42::Tags::MDEntryPx);
				bool isHandled = false;

				if (entry.contain(fix::FIX42::Tags::MDEntryRefID)) {
					
					const auto entryRefId
						= entry.getInt64(fix::FIX42::Tags::MDEntryRefID);
					if (entryRefId != 1) {
						isHandled = true;
						auto pos = security->m_book.find(entryRefId);
						if (pos == security->m_book.end()) {
							GetLog().Error(
								"Failed to delete price level with ref. ID %1% for %2%.",
								entryRefId,
								*security);
						} else {
							security->m_book.erase(pos);
						}
						security->m_book[entryId] = std::make_pair(
							entry.get(fix::FIX42::Tags::MDEntryType)
								== fix::FIX42::Values::MDEntryType::Bid,
							Security::Book::Level(
								entry.getDouble(fix::FIX42::Tags::MDEntryPx),
								ParseMdEntrySize(entry)));
					}
				
				}
				
				if (!isHandled) {

					auto pos = security->m_book.find(entryId);
					if (pos == security->m_book.end()) {
						GetLog().Error(
							"Failed to change price level"
								" with ID %1%, price %2% and qty %3% for %4%.",
							entryId,
							price,
							qty,
							*security);
						continue;
					}

					(*pos).second = std::make_pair(
						entry.get(fix::FIX42::Tags::MDEntryType)
							== fix::FIX42::Values::MDEntryType::Bid,
						Security::Book::Level(price, qty));

				}

			} else if (action == fix::FIX42::Values::MDUpdateAction::New) {

				const auto &qty = ParseMdEntrySize(entry);
				if (IsZero(qty)) {
					GetLog().Error(
						"Price level with zero-qty received for %1%: \"%2%\".",
						*security,
						message);
					continue;
				}

				security->m_book[entryId] = std::make_pair(
					entry.get(fix::FIX42::Tags::MDEntryType)
						== fix::FIX42::Values::MDEntryType::Bid,
					Security::Book::Level(
						entry.getDouble(fix::FIX42::Tags::MDEntryPx),
						qty));

			} else  if (action == fix::FIX42::Values::MDUpdateAction::Delete) {

				auto pos = security->m_book.find(entryId);
				if (pos == security->m_book.end()) {
					GetLog().Error(
						"Failed to delete price level with ID %1% for %2%.",
						entryId,
						*security);
					continue;
				}
				security->m_book.erase(pos);

			} else {

				AssertFail("Unknown entry type.");
				continue;

			}

		}

	} else if (message.type() == "W") {

		const fix::Group &entries
			= message.getGroup(fix::FIX42::Tags::NoMDEntries);
		AssertEq(1, entries.size());
		
		for (size_t i = 0; i < entries.size(); ++i) {
			
			const auto entryId = message.getInt64(fix::FIX42::Tags::MDEntryID);

			Assert(security->m_book.find(entryId) == security->m_book.end());
			
			security->m_book[entryId] = std::make_pair(
				message.get(fix::FIX42::Tags::MDEntryType)
					== fix::FIX42::Values::MDEntryType::Bid,
				Security::Book::Level(
					message.getDouble(fix::FIX42::Tags::MDEntryPx),
					ParseMdEntrySize(message)));

		}

	} else {
		
		return;
	
	}

	std::vector<Security::Book::Level> bids;
	bids.reserve(security->m_book.size());
		
	std::vector<Security::Book::Level> asks;
	asks.reserve(security->m_book.size());
		
	foreach(const auto &l, security->m_book) {
		if (l.second.first) {
			bids.emplace_back(l.second.second);
		} else {
			asks.emplace_back(l.second.second);
		}
	}

	std::sort(
		bids.begin(),
		bids.end(),
		[](
				const Security::Book::Level &lhs,
				const Security::Book::Level &rhs) {
			return lhs.GetPrice() > rhs.GetPrice();
		});
	std::sort(
		asks.begin(),
		asks.end(),
		[](
				const Security::Book::Level &lhs,
				const Security::Book::Level &rhs) {
			return lhs.GetPrice() < rhs.GetPrice();
		});

#	if defined(DEV_VER) && 0
		if (	
				GetTag() == "Currenex"
				/*&& security->GetSymbol().GetSymbol() == "USD/JPY"*/) {
			std::cout
				<< "############################### "
				<< *security << " " << security->GetSource().GetTag()
				<< std::endl;
			for (
					size_t i = 0;
					i < std::min(asks.size(), bids.size());
					++i) {
				std::cout
					<< bids[i].GetQty()
					<< ' ' << bids[i].GetPrice()
					<< "\t\t\t"
					<< asks[i].GetPrice()
					<< ' ' << asks[i].GetQty()
					<< std::endl;
			}
			std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
		}
#	endif

	AdjustBook(*security, bids, asks, message);

	if (
			security->m_bookMaxBookSize.first < bids.size()
			|| security->m_bookMaxBookSize.first / 4 >= bids.size()
			|| security->m_bookMaxBookSize.second < asks.size()
			|| security->m_bookMaxBookSize.second / 4 >= asks.size()) {
		security->m_bookMaxBookSize = std::make_pair(bids.size(), asks.size());
		GetTradingLog().Write(
			"book\tsize\t%1%\t%2%\t%3%\t%4%",
			[&](TradingRecord &record) {
				record
					% *security
					% security->m_bookMaxBookSize.first 
					% security->m_bookMaxBookSize.second
					% security->m_book.size();
			});
	}

	AssertGe(security->m_book.size(), bids.size() + asks.size());

	FixSecurity::BookUpdateOperation book = security->StartBookUpdate(now);
	book.GetBids().Swap(bids);
	book.GetAsks().Swap(asks);
	book.Commit(timeMeasurement);

}

void FixStream::AdjustBook(
		const FixSecurity &security,
		std::vector<Security::Book::Level> &bids,
		std::vector<Security::Book::Level> asks,
		const fix::Message &message)
		const {

	if (bids.empty() || asks.empty()) {
		return;
	}

	if (bids.front().GetPrice() <= asks.front().GetPrice()) {
		return;
	}

	GetTradingLog().Write(
		"book\tadjust\t%1%\t%2% <-> %3%\t%4%",
		[&](TradingRecord &record) {
			record
				% security
				% bids.front().GetPrice()
				% asks.front().GetPrice()
				% message.seqNum();
		});

	bids.front().Swap(asks.front());

	const auto &checkBook = [&](
			std::vector<Security::Book::Level> &side,
			bool isAsk) {
		
		if (side.size() == 1) {
			return;
		}
		
		auto &topLevel = side.front();
		auto &nextLevel = *(side.begin() + 1);
		if (IsEqual(topLevel.GetPrice(), nextLevel.GetPrice())) {
			nextLevel += topLevel;
			side.erase(side.begin());
		} else if (
				(isAsk && topLevel.GetPrice() > nextLevel.GetPrice())
					|| (!isAsk && topLevel.GetPrice() < nextLevel.GetPrice())) {
 			topLevel.Swap(nextLevel);
		}
	
	};

	checkBook(asks, true);
	checkBook(bids, false);

}

Qty FixStream::ParseMdEntrySize(const fix::GroupInstance &entry) const {
	return entry.getInt32(fix::FIX42::Tags::MDEntrySize);
}

Qty FixStream::ParseMdEntrySize(const fix::Message &message) const {
	return message.getInt32(fix::FIX42::Tags::MDEntrySize);
}
