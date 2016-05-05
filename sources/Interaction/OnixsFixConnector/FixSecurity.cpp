/*******************************************************************************
 *   Created: 2016/01/05 03:38:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixSecurity.hpp"
#include "Core/MarketDataSource.hpp"

namespace fix = OnixS::FIX;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

FixSecurity::FixSecurity(
		Context &context,
		const Symbol &symbol,
		const MarketDataSource &source)
	: Base(context, symbol, source, true)
	, m_flush(&FixSecurity::FlushBookIterativeUpdates) {
	//...//
}

void FixSecurity::OnSnapshotEntry(
		const pt::ptime &time,
		bool isBid,
		double price,
		const Qty &qty) {
	isBid
		?	m_snapshot.GetBid().Add(time, price, qty)
		:	m_snapshot.GetAsk().Add(time, price, qty);
	m_flush = &FixSecurity::FlushBookShanpshot;
}

void FixSecurity::OnNewEntry(
		const fix::Int64 &entryId,
		const pt::ptime &time,
		bool isBid,
		double price,
		const Qty &qty) {

	Assert(m_flush == &FixSecurity::FlushBookIterativeUpdates);

	const Entry order = {time, isBid, price, qty};
	if (!m_orderBook.emplace(entryId, std::move(order)).second) {
		GetSource().GetLog().Error(
			"Received entry with not unique new ID %1% (book size: %2%).",
			entryId,
			m_orderBook.size());
		AssertFail("Received entry with not unique ID.");
	}

	IncreaseNumberOfUpdates(isBid);

}

void FixSecurity::SetEntry(
		const fix::Int64 &entryId,
		const pt::ptime &time,
		bool isBid,
		double price,
		const Qty &qty) {
	Assert(m_flush == &FixSecurity::FlushBookIterativeUpdates);
	const Entry order = {time, isBid, price, qty};
	m_orderBook.emplace(entryId, std::move(order));
	IncreaseNumberOfUpdates(isBid);
}

void FixSecurity::OnEntryReplace(
		const fix::Int64 &prevEntryId,
		const fix::Int64 &newEntryId,
		const pt::ptime &time,
		bool isBid,
		double price,
		const Qty &qty) {

	AssertNe(prevEntryId, newEntryId);
	Assert(m_flush == &FixSecurity::FlushBookIterativeUpdates);

	const auto &pos = m_orderBook.find(prevEntryId);
	if (pos == m_orderBook.end()) {
		GetSource().GetLog().Error(
			"Failed to find entry %1% to replace.",
			prevEntryId);
		return;
	}
	Entry &order = pos->second;
	AssertLe(order.time, time);

	const bool isChanged
		= order.isUsed || order.isBid != isBid || !IsEqual(order.price, price);
	
	m_orderBook.erase(pos);
	{
		const Entry orderInfo = {time, isBid, price, qty};
		if (!m_orderBook.emplace(newEntryId, std::move(orderInfo)).second) {
			GetSource().GetLog().Error(
				"Received replace-entry with not unique new ID %1% -> %2%"
					" (book size: %3%).",
				prevEntryId,
				newEntryId,
				m_orderBook.size());
			AssertFail("Received entry with not unique ID.");
		}
	}

	if (isChanged) {
		IncreaseNumberOfUpdates(isBid);
	}

}

void FixSecurity::OnEntryUpdate(
		const fix::Int64 &entryId,
		const pt::ptime &time,
		bool isBid,
		double price,
		const Qty &qty) {

	Assert(m_flush == &FixSecurity::FlushBookIterativeUpdates);

	const auto &pos = m_orderBook.find(entryId);
	if (pos == m_orderBook.end()) {
		GetSource().GetLog().Error(
			"Failed to find entry %1% to update.",
			entryId);
		return;
	}
	Entry &order = pos->second;
	AssertLe(order.time, time);
	
	const bool isChanged
		= order.isUsed || order.isBid != isBid || !IsEqual(order.price, price);

	order.price = price;
	order.time = time;
	order.isBid = isBid;
	order.qty = qty;

	if (isChanged) {
		IncreaseNumberOfUpdates(isBid);
	}

}

void FixSecurity::OnEntryDelete(const fix::Int64 &entryId) {
	
	Assert(m_flush == &FixSecurity::FlushBookIterativeUpdates);

	const auto &pos = m_orderBook.find(entryId);
	if (pos == m_orderBook.end()) {
		GetSource().GetLog().Error(
			"Failed to find entry %1% to delete.",
			entryId);
		return;
	}
	
	const bool isBid = pos->second.isBid;
	const bool isUsed = pos->second.isUsed;
	m_orderBook.erase(pos);
	
	if (isUsed) {
		IncreaseNumberOfUpdates(isBid);
	}

}
		
void FixSecurity::ClearBook() {
	if (m_flush == &FixSecurity::FlushBookShanpshot) {
		m_snapshot.Clear();
	} else {
		m_orderBook.clear();
		m_hasBidUpdates
			= m_hasAskUpdates
			= true;
	}
}

void FixSecurity::Flush(
		const pt::ptime &time,
		const Lib::TimeMeasurement::Milestones &timeMeasurement) {
	(this->*m_flush)(time, timeMeasurement);
}

void FixSecurity::FlushBookIterativeUpdates(
		const pt::ptime &time,
		const TimeMeasurement::Milestones &timeMeasurement) {

	if (!m_hasBidUpdates && !m_hasAskUpdates) {
		return;
	}

	m_snapshot.SetTime(time);
	if (m_hasBidUpdates) {
		m_snapshot.GetBid().Clear();
	}
	if (m_hasAskUpdates) {
		m_snapshot.GetAsk().Clear();
	}

	foreach(auto &l, m_orderBook) {
		Entry &order = l.second;
		if (order.isBid) {
			if (m_hasBidUpdates) {
				order.isUsed = m_snapshot.GetBid().Update(
					order.time,
					order.price,
					order.qty);
			}
		} else if (m_hasAskUpdates) {
			order.isUsed = m_snapshot.GetAsk().Update(
				order.time,
				order.price,
				order.qty);
		}
	}

	SetBook(m_snapshot, timeMeasurement);

	m_hasBidUpdates
		= m_hasAskUpdates
		= false;

}

void FixSecurity::FlushBookShanpshot(
		const pt::ptime &time,
		const Lib::TimeMeasurement::Milestones &timeMeasurement) {
	AssertEq(0, m_orderBook.size());
	Assert(!m_hasBidUpdates);
	Assert(!m_hasAskUpdates);
	m_snapshot.SetTime(time);
	SetBook(m_snapshot, timeMeasurement);
	m_snapshot.Clear();
}

void FixSecurity::IncreaseNumberOfUpdates(bool isBid) throw() {
	(isBid ? m_hasBidUpdates : m_hasAskUpdates) = true;
}
