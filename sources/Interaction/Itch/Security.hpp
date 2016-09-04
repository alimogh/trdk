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
#include "Core/PriceBook.hpp"

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
			bool isUsed;
		};

	public:

		explicit Security(Context &, const Lib::Symbol &, MarketDataSource &);

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

		void ClearBook();

		void Flush(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);

	private:

		void IncreaseNumberOfUpdates(bool isBuy) throw();

	private:

		bool m_hasBidUpdates;
		bool m_hasAskUpdates;

		PriceBook m_snapshot;
		boost::unordered_map<Itch::OrderId, Order> m_orderBook;

		Itch::OrderId m_maxNewOrderId;

	};

} } }
