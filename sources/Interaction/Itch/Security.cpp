/**************************************************************************
 *   Created: 2015/03/19 00:44:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Itch;

namespace pt = boost::posix_time;

Itch::Security::Security(
		Context &context,
		const Lib::Symbol &symbol,
		const MarketDataSource &source,
		size_t maxBookLevelsCount)
	: Base(context, symbol, source),
	m_maxBookLevelsCount(maxBookLevelsCount),
	m_additionalBookLevelsCount(0),
	m_hasNewData(false) {
	StartLevel1();
}

void Itch::Security::OnNewOrder(
		const pt::ptime &time,
		bool isBuy,
		const Itch::OrderId &orderId,
		double price,
		double amount) {
	
	if (m_rawBook.find(orderId) != m_rawBook.end()) {
		GetContext().GetLog().Error(
			"Failed to add new order to quote book:"
				" order with ID %1% already exists (book size: %2%).",
			orderId,
			m_rawBook.size());
		throw ModuleError("Failed to add new order to quote book");
	}
	
	{
		const Order order = {time, isBuy, price, amount};
		m_rawBook.emplace(orderId, std::move(order));
	}

	m_hasNewData = true;

}

void Itch::Security::OnOrderModify(
		const pt::ptime &time,
		const Itch::OrderId &orderId,
		double amount) {
	
	const auto &it = m_rawBook.find(orderId);
	if (it == m_rawBook.end()) {
// 		GetContext().GetLog().Warn /*Error*/(
// 			"Failed to update order in quote book:"
// 				" filed to find order with ID %1% (book size: %2%).",
// 			orderId,
// 			m_rawBook.size());
//		throw ModuleError("Failed to update order in quote book");
		return;
	}
	
	AssertLe(it->second.time, time);
	it->second.time = time;
	
	AssertNe(amount, it->second.qty); // Can be removed.
	if (amount != it->second.qty) {
		it->second.qty = amount;
		m_hasNewData = true;
	}

}

void Itch::Security::OnOrderCancel(
		const pt::ptime &time,
		const Itch::OrderId &orderId) {

	const auto &it = m_rawBook.find(orderId);
	if (it == m_rawBook.end()) {
//		GetContext().GetLog().Warn /*Error*/(
// 			"Failed to delete order from quote book:"
// 				" filed to find order with ID %1% (book size: %2%).",
// 			orderId,
// 			m_rawBook.size());
//		throw ModuleError("Failed to delete order from quote book");
		return;
	}

	AssertLe(it->second.time, time);
	UseUnused(time);
	
	m_rawBook.erase(it);
	
	m_hasNewData = true;

}

template<typename SortFunc>
std::vector<Itch::Security::Book::Level> Itch::Security::SortBookSide(
		std::vector<Book::Level> &cache,
		const SortFunc &sortFunc,
		bool &isFull)
		const {
	
	isFull = true;

	std::vector<Book::Level> result;
	if (cache.empty()) {
		return result;
	}
	result.reserve(m_maxBookLevelsCount);

	std::sort(cache.begin(), cache.end(), sortFunc);

	for (auto i = cache.begin(); ; ) {
		
		const auto &next = i + 1;
		if (next == cache.end()) {
			break;
		}
		
		if (IsEqual(i->GetPrice(), next->GetPrice())) {
		
			*i += *next;
			cache.erase(next);
		
		} else {
		
			result.emplace_back(std::move(*i));
		
			if (
					result.size()
						>= m_maxBookLevelsCount + m_additionalBookLevelsCount) {
				AssertEq(
					result.size(),
					m_maxBookLevelsCount + m_additionalBookLevelsCount);
				isFull = false;
				break;
			}
	
			++i;
	
		}
	
	}

	return result;

}

size_t Itch::Security::CheckBookSideSizeAfterAdjusting(
		size_t sideSizeBeforAdjusting,
		std::vector<Book::Level> &side,
		const char *sideName,
		bool isFull)
		const {
	

	if (sideSizeBeforAdjusting <= m_maxBookLevelsCount) {
		AssertGe(sideSizeBeforAdjusting, side.size());
		return 0;
	}

	if (side.size() >= m_maxBookLevelsCount) {
		side.resize(m_maxBookLevelsCount);
		return 0;
	}

	if (isFull) {
		return 0;
	}
	
	AssertLt(side.size(), sideSizeBeforAdjusting);

	const auto additionalBookLevelsCount = m_maxBookLevelsCount - side.size();

	GetSource().GetLog().Debug(
		"Book too small after adjusting"
			" (%1% %2% price levels: %3% -> %4%).",
		GetSymbol().GetSymbol(),
		sideName,
		sideSizeBeforAdjusting,
		side.size());

	return additionalBookLevelsCount;

}

void Itch::Security::Flush(
		const pt::ptime &time,
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	if (!m_hasNewData) {
		return;
	}

	m_bidsCache.clear();
	m_asksCache.clear();
	foreach(const auto &l, m_rawBook) {
		if (l.second.isBuy) {
			m_bidsCache.emplace_back(
				l.second.time,
				l.second.price,
				l.second.qty);
		} else {
			m_asksCache.emplace_back(
				l.second.time,
				l.second.price,
				l.second.qty);
		}
	}

	for ( ; ; ) {

		bool isBidsFull;
		std::vector<Book::Level> bids = SortBookSide(
			m_bidsCache,
			[](const Book::Level &lhs, const Book::Level &rhs) {
				return lhs.GetPrice() > rhs.GetPrice();
			},
			isBidsFull);
		const auto bidsLevelsCountBeforeAdjusting = bids.size();
		
		bool isAsksFull;
		std::vector<Book::Level> asks = SortBookSide(
			m_asksCache,
			[](const Book::Level &lhs, const Book::Level &rhs) {
				return lhs.GetPrice() < rhs.GetPrice();
			},
			isAsksFull);
		const auto asksLevelsCountBeforeAdjusting = asks.size();

#		if 0
		{
			std::ostringstream os;
			os << std::endl;
			foreach_reversed (const auto &b, asks) {
				os << "\t\t" << b.GetPrice() << " | "<< b.GetQty() << std::endl;
			}
			foreach (const auto &b, bids) {
				os << b.GetQty() << " | " << b.GetPrice() << std::endl;
			}
			GetSource().GetLog().Debug(os.str().c_str());
		}
#		endif

		const bool isAdjusted = BookUpdateOperation::Adjust(*this, bids, asks);

		size_t additionalBookLevelsCount = std::max(
			CheckBookSideSizeAfterAdjusting(
				bidsLevelsCountBeforeAdjusting,
				bids,
				"bids",
				isBidsFull),
			CheckBookSideSizeAfterAdjusting(
				asksLevelsCountBeforeAdjusting,
				asks,
				"asks",
				isAsksFull));
		if (additionalBookLevelsCount) {
			m_additionalBookLevelsCount += additionalBookLevelsCount;
			continue;
		}

		{
			BookUpdateOperation book = StartBookUpdate(time, isAdjusted);
			book.GetBids().Swap(bids);
			book.GetAsks().Swap(asks);
			book.Commit(timeMeasurement);
		}

		break;

	}

	m_hasNewData = false;

}

void Itch::Security::ClearBook() {
	m_rawBook.clear();
}
