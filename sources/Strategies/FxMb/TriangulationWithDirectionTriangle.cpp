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
#include "Core/DropCopy.hpp"
#include "Core/RiskControl.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"

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
		const BestBidAskPairs &bestBidAskRef) 
	: m_strategy(strategy)
	, m_startTime(m_strategy.GetContext().GetCurrentTime())
	, m_bestBidAsk(bestBidAskRef)
	, m_report(*this, reportsState)
	, m_id(id)
	, m_y(y)
	, m_aQty(startQty) {

#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_pairsLegs.fill(nullptr);
#	endif
			
	m_pairs[PAIR_AB] = PairInfo(ab, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR_AB].leg] = &m_pairs[PAIR_AB];
	
	{
		auto &pair = m_pairs[PAIR_BC]; 
		pair = PairInfo(bc, m_bestBidAsk);
		m_pairsLegs[pair.leg] = &pair;
	}
			
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

	const auto qtyPrecision = 2;

	if (pair.isBuy) {

		auto qty = m_aQty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				if (security.GetAskQty() < qty) {
					throw HasNotMuchOpportunityException(security, qty);
				}
				break;
			case PAIR_BC:
				switch (pair.leg) {
					case LEG1:
						//! @sa TRDK-236
						qty *= GetPair(PAIR_AC).security->GetBidPrice();
						qty /= GetPair(PAIR_BC).security->GetAskPrice();
						break;
					case LEG2:
						AssertNe(LEG2, pair.leg);
						throw LogicError("BC can be leg 2");
					case LEG3:
						AssertNe(PAIR_BC, GetLeg(LEG1).GetPair());
						AssertNe(PAIR_BC, GetLeg(LEG2).GetPair());
						//! @sa TRDK-186.
						qty = (1 / price) * GetLeg(PAIR_AC).GetOpenedVolume();
						break;
					default:
						AssertEq(LEG1, pair.leg);
						throw Lib::LogicError("Unknown leg for pair");
				}
				if (security.GetAskQty() < qty / price) {
					throw HasNotMuchOpportunityException(security, qty);
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		Assert(!Lib::IsZero(security.GetAskPrice()));

		const Qty orderQty(Round(qty, qtyPrecision));

		result.reset(
			new Twd::LongPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				orderQty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				qtyPrecision));

		m_strategy.GetTradingLog().Write(
			"qty precision\tsell\t%1%\t%2%",
			[&qty, &orderQty](TradingRecord &record) {
				record % qty % orderQty;
			});

	} else {

		auto qty = m_aQty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				AssertLt(0, qty);
				if (security.GetBidQty() < qty) {
					throw HasNotMuchOpportunityException(security, qty);
				}
				break;
			case PAIR_BC:
				switch (pair.leg) {
					case LEG1:
						//! @sa TRDK-236
						qty *= GetPair(PAIR_AC).security->GetAskPrice();
						qty /= GetPair(PAIR_BC).security->GetBidPrice();
						break;
					case LEG2:
						AssertNe(LEG2, pair.leg);
						throw LogicError("BC can be leg 2");
					case LEG3:
						AssertNe(PAIR_BC, GetLeg(LEG1).GetPair());
						AssertNe(PAIR_BC, GetLeg(LEG2).GetPair());
						//! @sa TRDK-186.
						qty = (1 / price) * GetLeg(PAIR_AC).GetOpenedVolume();
						break;
					default:
						AssertEq(LEG1, pair.leg);
						throw Lib::LogicError("Unknown leg for pair");
				}
				if (security.GetBidQty() < qty / price) {
					throw HasNotMuchOpportunityException(security, qty);
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		AssertLt(0, qty);
		Assert(!Lib::IsZero(security.GetBidPrice()));

		const Qty orderQty(Round(qty, qtyPrecision));

		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				orderQty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				qtyPrecision));

		m_strategy.GetTradingLog().Write(
			"qty precision\tsell\t%1%\t%2%",
			[&qty, &orderQty](TradingRecord &record) {
				record % qty % orderQty;
			});

	}

	++pair.ordersCount;
		
	return result;
		
}

void Triangle::ReportStart() const {
	
	DropCopy *const dropCopy = m_strategy.GetContext().GetDropCopy();
	if (!dropCopy) {
		return;
	}

	dropCopy->ReportOperationStart(
		GetId(),
		GetStartTime(),
		m_strategy,
		m_strategy.CalcBookNumberOfUpdates());

}

void Triangle::ReportEnd() const {
	
	DropCopy *const dropCopy = m_strategy.GetContext().GetDropCopy();
	if (!dropCopy) {
		return;
	}
	
	const boost::shared_ptr<const FinancialResult> financialResult(
		new FinancialResult(m_strategy.GetRiskControlScope().TakeStatistics()));
	
	dropCopy->ReportOperationEnd(
		GetId(),
		m_strategy.GetContext().GetCurrentTime(),
		CalcYExecuted(),
		financialResult);

}