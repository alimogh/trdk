/**************************************************************************
 *   Created: 2014/10/16 01:06:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Util.hpp"
#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace OnixsHotspot {

	class HotspotSecurity : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit HotspotSecurity(
				Context &context,
				const Lib::Symbol &symbol,
				const MarketDataSource &source)
			: Base(context, symbol, source) {
			StartLevel1();
		}

	public:

		OnixS::HotspotItch::PriceLevels & GetAsksCache() {
			AssertEq(0, m_cache.asks.size());
			return m_cache.asks;
		}

		OnixS::HotspotItch::PriceLevels & GetBidsCache() {
			AssertEq(0, m_cache.bids.size());
			return m_cache.bids;
		}

	public:

		void Flush(
				const boost::posix_time::ptime &time,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {

			std::vector<Book::Level> bids;
			bids.reserve(m_cache.bids.size());
			foreach (const auto &it, m_cache.bids) {
				const auto &level = *it;
				bids.emplace_back(
					time,
					ConvertToDouble(level.price()),
					Qty(ConvertToDouble(level.amount())));
			}

			std::vector<Book::Level> asks;
			asks.reserve(m_cache.asks.size());
			foreach (const auto &it, m_cache.asks) {
				const auto &level = *it;
				asks.emplace_back(
					time,
					ConvertToDouble(level.price()),
					Qty(ConvertToDouble(level.amount())));
			}

			BookUpdateOperation book = StartBookUpdate(time, false);
			book.GetBids().Swap(bids);
			book.GetAsks().Swap(asks);
			book.Commit(timeMeasurement);

			m_cache.bids.clear();
			m_cache.asks.clear();

		}

	private:

		struct {
			OnixS::HotspotItch::PriceLevels asks;
			OnixS::HotspotItch::PriceLevels bids;
		} m_cache;
		

	};

} } }
