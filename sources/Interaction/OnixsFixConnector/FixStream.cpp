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
	: MarketDataSource(index, context, tag),
	m_isBookLogEnabled(conf.ReadBoolKey("log.book_adjust")),
	m_session(GetContext(), GetLog(), conf),
	m_isSubscribed(false),
	m_bookLevelsCount(
		conf.GetBase().ReadTypedKey<size_t>("General", "book.levels.count")) {
	GetLog().Info("Book size: %1% * 2 price levels.", m_bookLevelsCount);
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
	boost::shared_ptr<FixSecurity> result(
		new FixSecurity(GetContext(), symbol, *this, m_isBookLogEnabled));
	const_cast<FixStream *>(this)
		->m_securities.push_back(result);
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

		const auto &now = GetContext().GetCurrentTime();

		foreach (const auto &security, m_securities) {
			if (security->m_book.empty()) {
				continue;
			}
			GetTradingLog().Write(
				"book\terase\t%1%",
				[&](TradingRecord &record) {
					record % *security;
				});
			security->m_book.clear();

			FixSecurity::BookUpdateOperation book
				= security->StartBookUpdate(now, false);
			{
				std::vector<trdk::Security::Book::Level> empty;
				book.GetBids().Swap(empty);
			}
			{
				std::vector<trdk::Security::Book::Level> empty;
				book.GetAsks().Swap(empty);
			}
			book.Commit(TimeMeasurement::Milestones());

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
				const auto &action = entry.get(fix::FIX42::Tags::MDUpdateAction);

				if (action == fix::FIX42::Values::MDUpdateAction::Change) {

					const auto &qty = ParseMdEntrySize(entry);
					if (qty == 0) {
						GetLog().Warn(
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
									"Failed to delete price level with ref."
										" ID %1% for %2%.",
									entryRefId,
									*security);
							} else {
								security->m_book.erase(pos);
							}
							security->m_book[entryId] = std::make_pair(
								entry.get(fix::FIX42::Tags::MDEntryType)
									== fix::FIX42::Values::MDEntryType::Bid,
								Security::Book::Level(
									now,
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
							Security::Book::Level(now, price, qty));

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

					security->m_book[entryId] = std::make_pair(
						entry.get(fix::FIX42::Tags::MDEntryType)
							== fix::FIX42::Values::MDEntryType::Bid,
						Security::Book::Level(
							now,
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

	Assert(security);

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
	{
		if (	
				GetTag() == "Fastmatch"
				&& security->GetSymbol().GetSymbol() == "EUR/USD") {
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
	}
#	endif

	const auto &bidsLevelsCountBeforeAdjusting = bids.size();
	const auto &asksLevelsCountBeforeAdjusting = asks.size();

	const bool isAdjusted
		= Security::BookUpdateOperation::Adjust(*security, bids, asks);
	AssertGe(security->m_book.size(), bids.size() + asks.size());

	if (bids.size() > m_bookLevelsCount) {
		bids.resize(m_bookLevelsCount);
	} else if (
			bidsLevelsCountBeforeAdjusting >= m_bookLevelsCount
			&& bids.size() < m_bookLevelsCount) {
		AssertLt(bids.size(), bidsLevelsCountBeforeAdjusting);
		GetLog().Warn(
			"Book too small after adjusting"
				" (%1% bid price levels: %2% -> %3%).",
			security->GetSymbol().GetSymbol(),
			bidsLevelsCountBeforeAdjusting,
			bids.size());
	}

	if (asks.size() > m_bookLevelsCount) {
		asks.resize(m_bookLevelsCount);
	} else if (
			asksLevelsCountBeforeAdjusting >= m_bookLevelsCount
			&& asks.size() < m_bookLevelsCount) {
		AssertLt(asks.size(), asksLevelsCountBeforeAdjusting);
		GetLog().Warn(
			"Book too small after adjusting"
				" (%1% ask price levels: %2% -> %3%).",
			security->GetSymbol().GetSymbol(),
			asksLevelsCountBeforeAdjusting,
			asks.size());
	}

	if (
			security->m_sentBids == bids
			&& security->m_sentAsks == asks
			&& security->IsBookAdjusted() == isAdjusted) {
		return;
	}

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

	security->m_sentBids = bids;
	security->m_sentAsks = asks;

	FixSecurity::BookUpdateOperation book
		= security->StartBookUpdate(now, isAdjusted);
	book.GetBids().Swap(bids);
	book.GetAsks().Swap(asks);
	book.Commit(timeMeasurement);

	security->DumpAdjustedBook();

}

void FixStream::OnMarketDataSnapshot(
		const fix::Message &message,
		const boost::posix_time::ptime &dataTime,
		FixSecurity &security) {

	FixSecurity::BookSideSnapshot book;
			
	const fix::Group &entries
		= message.getGroup(fix::FIX42::Tags::NoMDEntries);
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


		const double price
			= entry.getDouble(fix::FIX42::Tags::MDEntryPx);
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

		//! @sa TRDK-237 - Quotes by one price from snapshots.
		// Assert(book.find(security.ScalePrice(price)) == book.end());
		book[security.ScalePrice(price)] = std::make_pair(
			entry.get(fix::FIX42::Tags::MDEntryType)
				== fix::FIX42::Values::MDEntryType::Bid,
			Security::Book::Level(dataTime, price, qty));

	}

	book.swap(security.m_book);

}

Qty FixStream::ParseMdEntrySize(const fix::GroupInstance &entry) const {
	return Qty(entry.getDouble(fix::FIX42::Tags::MDEntrySize));
}

Qty FixStream::ParseMdEntrySize(const fix::Message &message) const {
	return Qty(message.getDouble(fix::FIX42::Tags::MDEntrySize));
}
