/**************************************************************************
 *   Created: 2015/01/19 23:42:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirectionTriangle.hpp"
#include "TriangulationWithDirection.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

Triangle::Triangle(
		const Id &id,
		TriangulationWithDirection &strategy,
		ReportsState &reportsState,
		const Y &y,
		const Qty &startQty,
		const PairLegParams &ab,
		const PairLegParams &bc,
		const PairLegParams &ac,
		const BestBidAskPairs &bestBidAskRef,
		size_t bookUpdatesNumber) 
	: m_strategy(strategy),
	m_startTime(m_strategy.GetContext().GetCurrentTime()),
	m_bestBidAsk(bestBidAskRef),
	m_report(*this, reportsState),
	m_id(id),
	m_y(y),
	m_qtyStart(startQty),
	m_bookUpdatesNumber(bookUpdatesNumber) {

#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_pairsLegs.fill(nullptr);
#	endif
			
	m_pairs[PAIR_AB] = PairInfo(ab, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR_AB].leg] = &m_pairs[PAIR_AB];
			
	m_pairs[PAIR_BC] = PairInfo(bc, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR_BC].leg] = &m_pairs[PAIR_BC];
			
	m_pairs[PAIR_AC] = PairInfo(ac, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR_AC].leg] = &m_pairs[PAIR_AC];
		
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
	{
		foreach (const PairInfo &info, m_pairs) {
			foreach (const PairInfo &subInfo, m_pairs) {
				if (&subInfo == &info) {
					continue;
				}
				AssertNe(
					subInfo.security->GetSymbol(),
					info.security->GetSymbol());
				Assert(subInfo.security != info.security);
			}
		}

		foreach (const PairInfo *info, m_pairsLegs) {
			Assert(info);
		}

	}
#	endif

	//! @todo make setting for account currency
	m_conversionPricesBid = GetPair(PAIR_AB).security->GetBidPrice();
	if (Lib::IsZero(m_conversionPricesBid)) {
		throw HasNotMuchOpportunityException(*GetPair(PAIR_AB).security, 0);
	}
	m_conversionPricesAsk = GetPair(PAIR_AB).security->GetAskPrice();
	if (Lib::IsZero(m_conversionPricesAsk)) {
		throw HasNotMuchOpportunityException(*GetPair(PAIR_AB).security, 0);
	}

	UpdateYDirection();

}

Triangle::~Triangle() {
	//...//
}

boost::shared_ptr<Twd::Position> Triangle::CreateOrder(
		PairInfo &pair,
		Security &security,
		double price,
		const TimeMeasurement::Milestones &timeMeasurement) {

	AssertLt(0, price);

	boost::shared_ptr<Twd::Position> result;

	if (pair.isBuy) {

		double qty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				qty = m_qtyStart;
				AssertLt(0, qty);
				break;
			case PAIR_BC:
				qty = m_qtyStart * m_conversionPricesAsk;
				AssertLt(0, qty);
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		AssertLt(0, qty);
		if (security.GetAskQty() < qty) {
			throw HasNotMuchOpportunityException(security, Qty(qty));
		}
		Assert(!Lib::IsZero(security.GetAskPrice()));
			
		result.reset(
			new Twd::LongPosition(
				m_strategy,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetCashBaseCurrency(),
				//! @todo remove "to qty"
				Qty(qty),
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg));
			
	} else {

		double qty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				qty = m_qtyStart;
				AssertLt(0, qty);
				break;
			case PAIR_BC:
				qty = m_qtyStart * m_conversionPricesBid;
				AssertLt(0, qty);
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		AssertLt(0, qty);
		if (security.GetBidQty() < qty) {
			throw HasNotMuchOpportunityException(security, Qty(qty));
		}
		Assert(!Lib::IsZero(security.GetBidPrice()));
			
		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetCashBaseCurrency(),
				//! @todo remove "to qty"
				Qty(qty),
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg));
			
	}

	++pair.ordersCount;
		
	return result;
		
}
