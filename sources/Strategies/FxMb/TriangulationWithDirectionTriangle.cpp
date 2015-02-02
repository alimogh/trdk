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

boost::shared_ptr<Twd::Position> Triangle::CreateOrder(
		PairInfo &pair,
		Security &security,
		double price,
		const TimeMeasurement::Milestones &timeMeasurement) {
			
	boost::shared_ptr<Twd::Position> result;

	/*
		Let's say you want to invest 100 000 euros at each leg

		Buy eur.usd = 100 000 euros
		Buy usd.jpy = 100 000 euros x eur.usd ask (if base currency is euros)
		Sell eur.jpy = 100 000 euros x eur.jpy bid

		Let's reverse the directions

		Sell eur.usd = 100 000 euros x eur.usd bid (if base currency is euros)
		Sell usd.jpy = 100 000 x usd.jpy bid
		Buy eur.jpy = 100 000

	*/

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
			throw HasNotMuchOpportunityException();
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
		
		//! @todo remove "to qty"
		const Qty qty = Qty(m_qtyStart * m_conversionPricesBid);

		AssertLt(0, qty);
		if (security.GetBidQty() < qty) {
			throw HasNotMuchOpportunityException();
		}
		Assert(!Lib::IsZero(security.GetBidPrice()));
			
		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				m_strategy.GetContext().GetTradeSystem(
					security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetCashBaseCurrency(),
				qty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg));
			
	}

	++pair.ordersCount;
		
	return result;
		
}
