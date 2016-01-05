/*******************************************************************************
 *   Created: 2014/08/12 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/Security.hpp"
#include "Core/PriceBook.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FixSecurity : public Security {

	public:

		typedef trdk::Security Base;

	private:

		struct Entry {
			boost::posix_time::ptime time;
			bool isBid;
			double price;
			Qty qty;
			bool isUsed;
		};

	public:

		explicit FixSecurity(
				Context &context,
				const Lib::Symbol &symbol,
				const trdk::MarketDataSource &source);

	public:

		void OnSnapshotEntry(
				const boost::posix_time::ptime &,
				bool isBid,
				double price,
				const Qty &);
		void OnNewEntry(
				const OnixS::FIX::Int64 &entryId,
				const boost::posix_time::ptime &,
				bool isBid,
				double price,
				const Qty &);
		void OnEntryReplace(
				const OnixS::FIX::Int64 &prevEntryId,
				const OnixS::FIX::Int64 &newEntryId,
				const boost::posix_time::ptime &,
				bool isBid,
				double price,
				const Qty &);
		void OnEntryUpdate(
				const OnixS::FIX::Int64 &entryId,
				const boost::posix_time::ptime &,
				bool isBid,
				double price,
				const Qty &);
		void OnEntryDelete(const OnixS::FIX::Int64 &entryId);
		
		void ClearBook();

		void Flush(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);

	private:

		void IncreaseNumberOfUpdates(bool isBid) throw();

		void FlushBookShanpshot(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);
		void FlushBookIterativeUpdates(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);

	private:

		bool m_hasBidUpdates;
		bool m_hasAskUpdates;
		boost::unordered_map<OnixS::FIX::Int64, Entry> m_orderBook;
		
		PriceBook m_snapshot;

		void (FixSecurity::*m_flush)(
			const boost::posix_time::ptime &,
			const Lib::TimeMeasurement::Milestones &);

	};

} } }
