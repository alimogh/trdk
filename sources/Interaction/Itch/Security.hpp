/**************************************************************************
 *   Created: 2015/03/19 00:42:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace Itch {

	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;

	private:

		struct Order {
			boost::posix_time::ptime time;
			bool isBuy;
			double price;
			Qty qty;
		};

	public:

		explicit Security(
				Context &,
				const Lib::Symbol &,
				const MarketDataSource &,
				size_t maxBookLevelsCount);

	public:

		void OnNewOrder(
				const boost::posix_time::ptime &,
				bool isBuy,
				const Itch::OrderId &,
				double price,
				double amount);
		void OnOrderModify(
				const boost::posix_time::ptime &,
				const Itch::OrderId &,
				double amount);
		void OnOrderCancel(
				const boost::posix_time::ptime &,
				const Itch::OrderId &);

		void Flush(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);

		void ClearBook();

	private:

		template<typename SortFunc>
		std::vector<Book::Level> SortBookSide(
				std::vector<Book::Level> &cache,
				const SortFunc &,
				bool &isFull)
				const;

		size_t CheckBookSideSizeAfterAdjusting(
				size_t sideSizeBeforAdjusting,
				std::vector<Book::Level> &,
				const char *sideName,
				bool isFull)
				const;

	private:

		const size_t m_maxBookLevelsCount;
		//! Additional price levels to get required book size after adjusting.
		size_t m_additionalBookLevelsCount;

		bool m_hasNewData;
		
		std::map<Itch::OrderId, Order> m_rawBook;
		std::vector<Book::Level> m_bidsCache;
		std::vector<Book::Level> m_asksCache;

	};

} } }
