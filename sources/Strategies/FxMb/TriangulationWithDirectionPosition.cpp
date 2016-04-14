/**************************************************************************
 *   Created: 2015/08/05 02:58:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/


#include "Prec.hpp"
#include "TriangulationWithDirectionTriangle.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;


Twd::Position::Position(
		const Qty &qty,
		const Pair &pair,
		const Leg &leg,
		uint8_t qtyPrecision)
	: m_pair(pair)
	, m_leg(leg) {
	const_cast<OrderParams &>(m_orderParams).minTradeQty = qty;
	const_cast<OrderParams &>(m_orderParams).qtyPrecision = qtyPrecision;
}

Twd::LongPosition::LongPosition(
		Strategy &strategy,
		const Triangle &triangle,
		TradeSystem &tradeSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement,
		const Pair &pair,
		const Leg &leg,
		uint8_t qtyPrecision)
	: trdk::Position(
		strategy,
		triangle.GetId(),
		GetLegNo(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement)
	, Twd::Position(qty, pair, leg, qtyPrecision) {
	//...//
}

Twd::ShortPosition::ShortPosition(
		::Strategy &strategy,
		const Triangle &triangle,
		TradeSystem &tradeSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement,
		const Pair &pair,
		const Leg &leg,
		uint8_t qtyPrecision)
	: trdk::Position(
		strategy,
		triangle.GetId(),
		GetLegNo(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement)
	, Twd::Position(qty, pair, leg, qtyPrecision) {
	//...//
}
