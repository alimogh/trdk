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
	m_aQty(startQty),
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

		Currency currency;
		double qty = m_aQty;
		double baseCurrencyQty = qty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				currency = security.GetSymbol().GetCashBaseCurrency();
				if (security.GetAskQty() < qty) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			case PAIR_BC:
				switch (pair.leg) {
					case LEG1:
						currency = security.GetSymbol().GetCashBaseCurrency();
						qty *= GetPair(PAIR_AC).security->GetAskPrice();
						qty /= GetPair(PAIR_BC).security->GetAskPrice();
						baseCurrencyQty = qty;
						break;
					case LEG2:
						AssertNe(LEG2, pair.leg);
						throw LogicError("BC can be leg 2");
					case LEG3:
						AssertNe(PAIR_BC, GetLeg(LEG1).GetPair());
						AssertNe(PAIR_BC, GetLeg(LEG2).GetPair());
						currency = security.GetSymbol().GetCashQuoteCurrency();
						qty = GetLeg(LEG1).GetOpenedVolume();
						if (GetLeg(LEG1).GetPair() == PAIR_AB) {
							qty *= GetPair(PAIR_BC).security->GetAskPrice();
							baseCurrencyQty = GetLeg(LEG1).GetOpenedVolume();
						} else {
							baseCurrencyQty = qty / price;
						}
						break;
				}
				if (security.GetAskQty() < qty / price) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		Assert(!Lib::IsZero(security.GetAskPrice()));
			
		result.reset(
			new Twd::LongPosition(
				m_strategy,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				currency,
				//! @todo remove "to qty"
				//! @todo see TRDK-92
				Qty(boost::math::round(qty)),
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				//! @todo remove "to qty"
				//! @todo see TRDK-92
				//! @todo see TRDK-4
				Qty(floor(baseCurrencyQty))));
			
	} else {

		Currency currency;
		double qty = qty = m_aQty;
		double baseCurrencyQty = qty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				currency = security.GetSymbol().GetCashBaseCurrency();
				AssertLt(0, qty);
				if (security.GetBidQty() < qty) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			case PAIR_BC:
				switch (pair.leg) {
					case LEG1:
						currency = security.GetSymbol().GetCashBaseCurrency();
						qty *= GetPair(PAIR_AC).security->GetBidPrice();
						qty /= GetPair(PAIR_BC).security->GetBidPrice();
						baseCurrencyQty = qty;
						break;
					case LEG2:
						AssertNe(LEG2, pair.leg);
						throw LogicError("BC can be leg 2");
					case LEG3:
						AssertNe(PAIR_BC, GetLeg(LEG1).GetPair());
						AssertNe(PAIR_BC, GetLeg(LEG2).GetPair());
						currency = security.GetSymbol().GetCashQuoteCurrency();
						qty = GetLeg(LEG1).GetOpenedVolume();
						if (GetLeg(LEG1).GetPair() == PAIR_AB) {
							qty *= GetPair(PAIR_BC).security->GetBidPrice();
							baseCurrencyQty = GetLeg(LEG1).GetOpenedVolume();
						} else {
							baseCurrencyQty = qty / price;
						}
						break;
				}
				if (security.GetBidQty() < qty / price) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		AssertLt(0, qty);
		Assert(!Lib::IsZero(security.GetBidPrice()));
			
		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				currency,
				//! @todo remove "to qty"
				//! @todo see TRDK-92
				Qty(boost::math::round(qty)),
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				//! @todo remove "to qty"
				//! @todo see TRDK-92
				//! @todo see TRDK-4
				Qty(floor(baseCurrencyQty))));
			
	}

	++pair.ordersCount;
		
	return result;
		
}
