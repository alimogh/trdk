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

Triangle::~Triangle() {
	AssertEq(1, m_strategy.GetPositions().GetSize());
	AssertNe(
		LEG2,
		dynamic_cast<Twd::Position &>(*m_strategy.GetPositions().GetBegin()).GetLeg());
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
				m_strategy.GetContext().GetTradeSystem(
					security.GetSource().GetIndex()),
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
				m_strategy.GetContext().GetTradeSystem(
					security.GetSource().GetIndex()),
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
