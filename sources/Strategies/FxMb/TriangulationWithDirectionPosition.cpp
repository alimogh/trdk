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
		const Qty &baseCurrencyQty)
	: trdk::Position(
		strategy,
		triangle.GetId(),
		uint64_t(pair),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement),
	m_pair(pair),
	m_leg(leg) {
	const_cast<OrderParams &>(m_orderParams).minTradeQty = baseCurrencyQty;
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
		const Qty &baseCurrencyQty)
	: trdk::Position(
		strategy,
		triangle.GetId(),
		uint64_t(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement),
	Twd::Position(
		strategy,
		triangle,
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement,
		pair,
		leg,
		baseCurrencyQty),
	trdk::LongPosition(
		strategy,
		triangle.GetId(),
		uint64_t(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement) {
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
		const Qty &baseCurrencyQty)
	: trdk::Position(
		strategy,
		triangle.GetId(),
		uint64_t(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement),
	Twd::Position(
		strategy,
		triangle,
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement,
		pair,
		leg,
		baseCurrencyQty),
	trdk::ShortPosition(
		strategy,
		triangle.GetId(),
		uint64_t(leg),
		tradeSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement) {
	//...//
}
