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

		void Flush(const Lib::TimeMeasurement::Milestones &timeMeasurement) {

			if (!m_cache.asks.empty() && !m_cache.bids.empty()) {

				SetLevel1(
					boost::posix_time::microsec_clock::universal_time(),
					Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
						ScalePrice(ConvertToDouble(m_cache.bids[0]->price()))),
					Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
						ConvertToUInt(m_cache.bids[0]->amount())),
					Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
						ScalePrice(ConvertToDouble(m_cache.asks[0]->price()))),
					Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
						ConvertToUInt(m_cache.asks[0]->amount())),
					timeMeasurement);

				m_cache.asks.clear();
				m_cache.bids.clear();

			} else if (!m_cache.asks.empty())  {

				AssertEq(0, m_cache.bids.size());

				SetLevel1(
					boost::posix_time::microsec_clock::universal_time(),
					Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
						ScalePrice(ConvertToDouble(m_cache.asks[0]->price()))),
					Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
						ConvertToUInt(m_cache.asks[0]->amount())),
					timeMeasurement);

				m_cache.asks.clear();

			} else if (!m_cache.bids.empty())  {

				AssertEq(0, m_cache.asks.size());
				
				SetLevel1(
					boost::posix_time::microsec_clock::universal_time(),
					Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
						ScalePrice(ConvertToDouble(m_cache.bids[0]->price()))),
					Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
						ConvertToUInt(m_cache.bids[0]->amount())),
					timeMeasurement);

				m_cache.bids.clear();

			}

		}

	private:

		struct {
			OnixS::HotspotItch::PriceLevels asks;
			OnixS::HotspotItch::PriceLevels bids;
		} m_cache;
		

	};

} } }
