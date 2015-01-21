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
		const Leg &leg,
		Security &security,
		double price,
		const Qty &qty,
		const TimeMeasurement::Milestones &timeMeasurement) {
			
	boost::shared_ptr<Twd::Position> result;
			
	PairInfo &pair = GetPair(leg);

	const auto &currency = !pair.isBaseCurrency
		?	security.GetSymbol().GetCashQuoteCurrency()
		:	security.GetSymbol().GetCashBaseCurrency();
			
	if (pair.isBuy) {

		if (security.GetAskQty() == 0) {
			throw HasNotMuchOpportunityException();
		}
		Assert(!Lib::IsZero(security.GetAskPrice()));
			
		result.reset(
			new Twd::LongPosition(
				m_strategy,
				m_strategy.GetContext().GetTradeSystem(
					security.GetSource().GetIndex()),
				security,
				currency,
				qty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg));
			
	} else {
			
		if (security.GetBidQty() == 0) {
			throw HasNotMuchOpportunityException();
		}
		Assert(!Lib::IsZero(security.GetBidPrice()));
			
		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				m_strategy.GetContext().GetTradeSystem(
					security.GetSource().GetIndex()),
				security,
				currency,
				qty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg));
			
	}

	++pair.ordersCount;
		
	return result;
		
}
