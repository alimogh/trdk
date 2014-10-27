/**************************************************************************
 *   Created: 2014/08/12 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FixSecurity : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit FixSecurity(
					Context &context,
					const Lib::Symbol &symbol,
					const trdk::MarketDataSource &source)
				: Base(context, symbol, source) {
			StartLevel1();
		}

	public:

		void SetBid(
					double price,
					const Qty &qty,
					const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			SetLevel1(
				boost::posix_time::microsec_clock::universal_time(),
				Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
					ScalePrice(price)),
				Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(qty),
				timeMeasurement);
		}

		void SetOffer(
					double price,
					const Qty &qty,
					const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			SetLevel1(
				boost::posix_time::microsec_clock::universal_time(),
				Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
					ScalePrice(price)),
				Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(qty),
				timeMeasurement);
		}

	};

} } }
