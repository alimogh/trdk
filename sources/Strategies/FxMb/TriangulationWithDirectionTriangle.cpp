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
		PairSetData &pairsDataRef) 
	: m_strategy(strategy)
	, m_startTime(m_strategy.GetContext().GetCurrentTime())
	, m_pairsData(pairsDataRef)
	, m_report(*this, reportsState)
	, m_id(id)
	, m_y(y)
	, m_aQty(startQty) {

#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_pairsLegs.fill(nullptr);
#	endif
			
	m_pairs[PAIR_AB] = PairInfo(ab, m_pairsData);
	m_pairsLegs[m_pairs[PAIR_AB].leg] = &m_pairs[PAIR_AB];
	
	{
		auto &pair = m_pairs[PAIR_BC]; 
		pair = PairInfo(bc, m_pairsData);
		m_pairsLegs[pair.leg] = &pair;
	}
			
	m_pairs[PAIR_AC] = PairInfo(ac, m_pairsData);
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

		double qty = m_aQty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				if (security.GetAskQty() < qty) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
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
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		Assert(!Lib::IsZero(security.GetAskPrice()));

		//! @sa TRDK-235
		AssertGt(1.0, pair.pairData->unsentQtyPrecisionVolume);
		AssertLt(-1.0, pair.pairData->unsentQtyPrecisionVolume);
		auto cleanQty = Qty(qty);
		const auto unsentQtyPrecision = qty - double(cleanQty);
		const auto additionalQty
			= Qty(pair.pairData->unsentQtyPrecisionVolume + unsentQtyPrecision);
		AssertLe(0, additionalQty);

		result.reset(
			new Twd::LongPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				cleanQty + additionalQty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				unsentQtyPrecision,
				additionalQty));
			
	} else {

		double qty = m_aQty;
		switch (pair.id) {
			case PAIR_AB:
			case PAIR_AC:
				AssertLt(0, qty);
				if (security.GetBidQty() < qty) {
					throw HasNotMuchOpportunityException(security, Qty(qty));
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
					throw HasNotMuchOpportunityException(security, Qty(qty));
				}
				break;
			default:
				AssertEq(PAIR_AB, pair.id);
				throw Lib::LogicError("Unknown pair for leg");
		}

		AssertLt(0, qty);
		Assert(!Lib::IsZero(security.GetBidPrice()));

		//! @sa TRDK-235
		AssertGt(1.0, pair.pairData->unsentQtyPrecisionVolume);
		AssertLt(-1.0, pair.pairData->unsentQtyPrecisionVolume);
		auto cleanQty = Qty(qty);
		const auto unsentQtyPrecision = qty - double(cleanQty);
		const auto additionalQty
			= -Qty(pair.pairData->unsentQtyPrecisionVolume - unsentQtyPrecision);
		AssertGe(0, additionalQty);

		result.reset(
			new Twd::ShortPosition(
				m_strategy,
				*this,
				m_strategy.GetTradeSystem(security.GetSource().GetIndex()),
				security,
				security.GetSymbol().GetFotBaseCurrency(),
				cleanQty + additionalQty,
				security.ScalePrice(price),
				timeMeasurement,
				pair.id,
				pair.leg,
				-unsentQtyPrecision,
				additionalQty));
			
	}

	return result;
		
}

void Triangle::OnOrderSent(const boost::shared_ptr<Twd::Position> &order) {

	PairInfo &pair = GetPair(order->GetLeg());

	pair.pairData->unsentQtyPrecisionVolume += order->GetUnsentQtyPrecision();
	pair.pairData->unsentQtyPrecisionVolume
		-= order->GetAdditionalQtyFromPrevOrders();

	if (
			!IsZero(order->GetUnsentQtyPrecision())
			|| !IsZero(order->GetAdditionalQtyFromPrevOrders())) {
		m_strategy.GetTradingLog().Write(
			"qty precision\torder\t%1%\t%2%"
				"\tqty: %3% + %4% = %5%"
				"\tuncovered: %6% + (%7%) - (%8%) = %9%",
			[&](TradingRecord &record) {
				record
					% order->GetSecurity().GetSymbol().GetSymbol()
					% order->GetTypeStr();
				record
					% (order->GetPlanedQty()
						- order->GetAdditionalQtyFromPrevOrders())
					% order->GetAdditionalQtyFromPrevOrders()
					% order->GetPlanedQty();
				record
					% (pair.pairData->unsentQtyPrecisionVolume
						+ order->GetAdditionalQtyFromPrevOrders()
						- order->GetUnsentQtyPrecision())
					% order->GetUnsentQtyPrecision()
					% order->GetAdditionalQtyFromPrevOrders()
					% pair.pairData->unsentQtyPrecisionVolume;
			});
	}

	m_legs[pair.leg] = order;

	m_lastOrderParams.Set(
		pair.leg,
		order->GetSecurity(),
		order->GetSecurity().DescalePrice(order->GetOpenPrice()));

	++pair.ordersCount;

}

void Triangle::OnOrderCanceled(boost::shared_ptr<Twd::Position> &leg) {
	
	Assert(leg);
	
	PairInfo &pair = GetPair(leg->GetLeg());

	pair.pairData->unsentQtyPrecisionVolume -= leg->GetUnsentQtyPrecision();
	pair.pairData->unsentQtyPrecisionVolume
		+= leg->GetAdditionalQtyFromPrevOrders();

	if (
			!IsZero(leg->GetUnsentQtyPrecision())
			|| !IsZero(leg->GetAdditionalQtyFromPrevOrders())) {
		m_strategy.GetTradingLog().Write(
			"qty precision\trollback\t%1%\t%2%"
				"\tqty: %3% + %4% = %5%"
				"\tuncovered: %6% - (%7%) + (%8%) = %9%",
			[&](TradingRecord &record) {
				record
					% leg->GetSecurity().GetSymbol().GetSymbol()
					% leg->GetTypeStr();
				record
					% (
						leg->GetPlanedQty()
						- leg->GetAdditionalQtyFromPrevOrders())
					% leg->GetAdditionalQtyFromPrevOrders()
					% leg->GetPlanedQty();
				record
					% (pair.pairData->unsentQtyPrecisionVolume
						- leg->GetAdditionalQtyFromPrevOrders()
						+ leg->GetUnsentQtyPrecision())
					% leg->GetUnsentQtyPrecision()
					% leg->GetAdditionalQtyFromPrevOrders()
					% pair.pairData->unsentQtyPrecisionVolume;
			});
	}

	leg.reset();

}

void Triangle::OnLeg1Cancel() {

	Assert(IsLegStarted(LEG1));
	Assert(!IsLegStarted(LEG2));
	Assert(!IsLegStarted(LEG3));

	Assert(!GetLeg(LEG1).IsOpened());
	Assert(!GetLeg(LEG2).IsOpened());

	OnOrderCanceled(m_legs[LEG1]);

}

void Triangle::OnLeg2Cancel() {

	Assert(IsLegStarted(LEG1));
	Assert(IsLegStarted(LEG2));
	Assert(!IsLegStarted(LEG3));

	Assert(GetLeg(LEG1).IsOpened());
	Assert(!GetLeg(LEG2).IsOpened());

	OnOrderCanceled(m_legs[LEG2]);

}

void Triangle::OnLeg3Cancel() {

	Assert(IsLegStarted(LEG1));
	Assert(IsLegStarted(LEG2));
	Assert(IsLegStarted(LEG3));

	Assert(GetLeg(LEG1).IsOpened());
	Assert(GetLeg(LEG2).IsOpened());
	Assert(!GetLeg(LEG3).IsOpened());

	OnOrderCanceled(m_legs[LEG3]);

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