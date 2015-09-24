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
#include "Y.hpp"
#include "Core/DropCopy.hpp"
#include "Core/RiskControl.hpp"
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
		const PairLegParams &pair1,
		const PairLegParams &pair2,
		const PairLegParams &pair3,
		const BestBidAskPairs &bestBidAskRef) 
	: m_strategy(strategy)
	, m_startTime(m_strategy.GetContext().GetCurrentTime())
	, m_bestBidAsk(bestBidAskRef)
	, m_report(*this, reportsState)
	, m_id(id)
	, m_y(y) {

#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_pairsLegs.fill(nullptr);
#	endif

	m_pairs[PAIR1] = PairInfo(pair1, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR1].leg] = &m_pairs[PAIR1];
	
	m_pairs[PAIR2] = PairInfo(pair2, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR2].leg] = &m_pairs[PAIR2];
			
	m_pairs[PAIR3] = PairInfo(pair3, m_bestBidAsk);
	m_pairsLegs[m_pairs[PAIR3].leg] = &m_pairs[PAIR3];
		
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

bool Triangle::HasOpportunity() const {
	return CheckOpportunity(m_yDirection) == GetY();
}

void Triangle::ResetYDirection() {
	Twd::ResetYDirection(m_yDirection);
}

void Triangle::UpdateYDirection() {
	m_strategy.GetProduct().CalcYDirection(
		GetCalcSecurity(PAIR1),
		GetCalcSecurity(PAIR2),
		GetCalcSecurity(PAIR3),
		m_yDirection);
}

double Triangle::CalcYTargeted() const {

	const auto getPrice = [this](const Pair &pair) -> double {
		if (IsLegStarted(pair)) {
			const Twd::Position &leg = GetLeg(pair);
			const auto &result
				= leg
					.GetSecurity()
					.DescalePrice(leg.GetOpenStartPrice());
			Assert(!Lib::IsZero(result));
			return result;
		} else {
			const PairInfo &pairInfo = GetPair(pair);
			AssertNe(LEG1, pairInfo.leg);
			return pairInfo.GetCurrentBestPrice();
		}
	};

	if (m_y ==  Y1) {
		return m_strategy.GetProduct().CalcY1(
			getPrice(PAIR1),
			getPrice(PAIR2),
			getPrice(PAIR3));
	} else {
		AssertEq(Y2, m_y);
		return m_strategy.GetProduct().CalcY2(
			getPrice(PAIR1),
			getPrice(PAIR2),
			getPrice(PAIR3));
	}

}

double Triangle::CalcYExecuted() const {
			
	Assert(
		IsLegStarted(LEG1)
		&& IsLegStarted(LEG2)
		&& IsLegStarted(LEG3));
	Assert(
		GetLeg(LEG1).IsOpened()
		&& GetLeg(LEG2).IsOpened()
		&& GetLeg(LEG3).IsOpened());
	Assert(
		!GetLeg(LEG1).IsClosed()
		&& !GetLeg(LEG2).IsClosed()
		&& !GetLeg(LEG3).IsClosed());

	const auto getPrice = [this](const Pair &pair) -> double {
		const Twd::Position &leg = GetLeg(pair);
		const auto &result
			= leg.GetSecurity().DescalePrice(leg.GetOpenPrice());
		Assert(!Lib::IsZero(result));
		return result;
	};

	if (m_y ==  Y1) {
		return m_strategy.GetProduct().CalcY1(
			getPrice(PAIR1),
			getPrice(PAIR2),
			getPrice(PAIR3));
	} else {
		AssertEq(Y2, m_y);
		return m_strategy.GetProduct().CalcY2(
			getPrice(PAIR1),
			getPrice(PAIR2),
			getPrice(PAIR3));
	}
		
}

boost::shared_ptr<Twd::Position> Triangle::CreateOrder(
		PairInfo &pair,
		Security &security,
		double price,
		const TimeMeasurement::Milestones &timeMeasurement) {

	AssertLt(0, price);

	boost::shared_ptr<Twd::Position> result;

	const auto &qty = m_strategy.GetProduct().CalcOrderQty(
		price,
		pair.isBuy,
		pair.id,
		pair.leg,
		*this);
	AssertLt(0, qty);

	if (pair.isBuy) {

		if (
				security.GetAskQty() < qty
				|| Lib::IsZero(security.GetAskPrice())) {
			throw HasNotMuchOpportunityException(security, qty);
		}

		result.reset(
			new Twd::LongPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				qty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				qty));
			
	} else {

		if (
				security.GetBidQty() < qty
				|| Lib::IsZero(security.GetBidPrice())) {
			throw HasNotMuchOpportunityException(security, qty);
		}

		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				qty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				qty));

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
		m_strategy.CalcBookUpdatesNumber());

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