/**************************************************************************
 *   Created: 2015/01/10 18:14:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TriangulationWithDirectionReport.hpp"
#include "TriangulationWithDirectionPosition.hpp"
#include "TriangulationWithDirectionTypes.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Y.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	class Triangle : private boost::noncopyable {

	public:

		typedef size_t Id;

		struct PairLegParams {
			Pair id;
			Leg leg;
			bool isBuy;
			bool isBaseCurrency;
		};

		struct PairInfo : public PairLegParams {
			
			const BestBidAsk *bestBidAsk;

			size_t ordersCount;

			PairInfo() {
				//...//
			}

			explicit PairInfo(
					const PairLegParams &params,
					const BestBidAskPairs &bestBidAskRef)
				: PairLegParams(params),
				bestBidAsk(&bestBidAskRef[id]),
				ordersCount(0) {	
			}

			Security & GetBestSecurity() {
				const Security &security = bestBidAsk->service->GetSecurity(
					!isBuy
						?	bestBidAsk->bestBid.source
						:	bestBidAsk->bestAsk.source);
				//! @todo FIXME (const_cast)
				return const_cast<Security &>(security);
			}

		};

	public:

		explicit Triangle(
				TriangulationWithDirection &strategy,
				ReportsState &reportsState,
				const Id &id,
				const Y &y,
				const Qty &startQty,
				const PairLegParams ab,
				const PairLegParams bc,
				const PairLegParams ac,
				const BestBidAskPairs &bestBidAskRef) 
			: m_strategy(strategy),
			m_bestBidAsk(bestBidAskRef),
			m_report(*this, reportsState),
			m_id(id),
			m_y(y),
			m_qtyStart(startQty),
			m_qtyLeg2(0) {

#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				m_pairsLegs.fill(nullptr);
#			endif
			
			m_pairs[PAIR_AB] = PairInfo(ab, m_bestBidAsk);
			m_pairsLegs[m_pairs[PAIR_AB].leg] = &m_pairs[PAIR_AB];
			
			m_pairs[PAIR_BC] = PairInfo(bc, m_bestBidAsk);
			m_pairsLegs[m_pairs[PAIR_BC].leg] = &m_pairs[PAIR_BC];
			
			m_pairs[PAIR_AC] = PairInfo(ac, m_bestBidAsk);
			m_pairsLegs[m_pairs[PAIR_AC].leg] = &m_pairs[PAIR_AC];
		
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				foreach (const PairInfo *info, m_pairsLegs) {
					Assert(info);
				}
#			endif

			UpdateYDirection();

		}

	public:

		void StartLeg1(
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const boost::array<PairSpeed, numberOfPairs> &pairsSpeed) {
		
			Assert(!m_legs[LEG1]);
			Assert(!m_legs[LEG2]);
			Assert(!m_legs[LEG3]);
			if (m_legs[LEG1] || m_legs[LEG2] || m_legs[LEG3]) {
				throw Lib::LogicError(
					"Failed to start triangle leg 1 (wrong strategy logic)");
			}

			boost::shared_ptr<Twd::Position> order = CreateOrder(
				LEG1,						
				m_qtyStart,
				timeMeasurement);
			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_START);

			order->OpenAtStartPrice();
			m_legs[LEG1] = order;

			m_report.ReportAction(
				"detected",
				"signal",
				"1",
				nullptr,
				&pairsSpeed);
			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_DECISION_STOP);

		}

		void StartLeg2() {

			Assert(m_legs[LEG1]);
			Assert(!m_legs[LEG2]);
			Assert(!m_legs[LEG3]);

			Assert(m_legs[LEG1]->IsOpened());
			Assert(!m_legs[LEG1]->IsClosed());

			if (
					!m_legs[LEG1]
					|| !m_legs[LEG1]->IsOpened()
					|| m_legs[LEG1]->IsClosed()
					|| m_legs[LEG2]
					|| m_legs[LEG3]) {
				throw Lib::LogicError(
					"Failed to start triangle leg 2 (wrong strategy logic)");
			}

			if (!m_qtyLeg2) {

				const auto &leg1Security = m_legs[LEG1]->GetSecurity();
				const double leg1Price
					= leg1Security.DescalePrice(m_legs[LEG1]->GetOpenPrice());
				const auto leg1Vol = leg1Price * m_legs[LEG1]->GetOpenedQty();

				const PairInfo &leg3Pair = GetPair(LEG3);
				const double leg3Price = leg3Pair.isBuy
					?	leg3Pair.bestBidAsk->bestAsk.price
					:	leg3Pair.bestBidAsk->bestBid.price;

				const auto &leg3QuoteCurrency
					= leg3Pair
						.bestBidAsk
						->service
						->GetSecurity(0)
						.GetSymbol()
						.GetCashQuoteCurrency();
				const auto &leg1QuoteCurrency
					= leg1Security.GetSymbol().GetCashQuoteCurrency();

				m_qtyLeg2 = leg3QuoteCurrency == leg1QuoteCurrency
					//! @todo remove "to qty"
					?	Qty(leg1Vol / leg3Price)
					:	Qty(leg1Vol * leg3Price);

			}

			boost::shared_ptr<Twd::Position> order = CreateOrder(
				LEG2,
				m_qtyLeg2,
				Lib::TimeMeasurement::Milestones());
			order->OpenAtStartPrice();

			m_legs[LEG2] = order;

		}

		void StartLeg3(const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			StartLeg3(GetPair(LEG3).GetBestSecurity(), timeMeasurement);
		}

		void StartLeg3(
				Security &security,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {

			Assert(m_legs[LEG1]);
			Assert(m_legs[LEG2]);
			Assert(!m_legs[LEG3]);

			Assert(m_legs[LEG1]->IsOpened());
			Assert(!m_legs[LEG1]->IsClosed());

			Assert(m_legs[LEG2]->IsOpened());
			Assert(!m_legs[LEG2]->IsClosed());

			if (
					!m_legs[LEG1]
					|| !m_legs[LEG1]->IsOpened()
					|| m_legs[LEG1]->IsClosed()
					|| !m_legs[LEG2]
					|| !m_legs[LEG2]->IsOpened()
					|| m_legs[LEG2]->IsClosed()
					|| m_legs[LEG3]) {
				throw Lib::LogicError(
					"Failed to start triangle leg 3 (wrong strategy logic)");
			}

			const double leg1Price = m_legs[LEG1]->GetSecurity().DescalePrice(
				m_legs[LEG1]->GetOpenPrice());
			const auto leg1Vol = leg1Price * m_legs[LEG1]->GetOpenedQty();

			const boost::shared_ptr<Twd::Position> order = CreateOrder(
				LEG3,
				security,
				//! @todo remove "to qty"
				Qty(leg1Vol),
				timeMeasurement);

			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_START);

			order->OpenAtStartPrice();
			m_legs[LEG3] = order;

			m_report.ReportAction("detected", "signal", order->GetLeg());
			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_DECISION_STOP);

		}

		void Cancel(const Position::CloseType &closeType) {
			
			Assert(m_legs[LEG1]);
			Assert(!m_legs[LEG2]);
			Assert(!m_legs[LEG3]);

			Assert(!m_legs[LEG1]->HasActiveOrders());

			if (!m_legs[LEG1] || m_legs[LEG2] || m_legs[LEG3]) {
				throw Lib::LogicError(
					"Failed to cancel triangle (wrong strategy logic)");
			}

			m_legs[LEG1]->CloseAtCurrentPrice(closeType);

		}

	public:

		void OnLeg2Cancel() {
			
			Assert(m_legs[LEG1]);
			Assert(m_legs[LEG2]);
			Assert(!m_legs[LEG3]);

			Assert(m_legs[LEG1]->IsOpened());
			Assert(!m_legs[LEG2]->IsOpened());

			m_legs[LEG2].reset();

		}

		void OnLeg3Cancel() {
			
			Assert(m_legs[LEG1]);
			Assert(m_legs[LEG2]);
			Assert(m_legs[LEG3]);

			Assert(m_legs[LEG1]->IsOpened());
			Assert(m_legs[LEG2]->IsOpened());
			Assert(!m_legs[LEG3]->IsOpened());

			m_legs[LEG3].reset();
		
		}

	public:

		const Id & GetId() const {
			return m_id;
		}

		const Y & GetY() const {
			return m_y;
		}

		TriangleReport & GetReport() {
			return m_report;
		}

		Twd::Position & GetLeg(const Leg &leg) {
			Assert(m_legs[leg]);
			return *m_legs[leg];  
		}
		const Twd::Position & GetLeg(const Leg &leg) const {
			return const_cast<Triangle *>(this)->GetLeg(leg);
		}

		Twd::Position & GetLeg(const Pair &pair) {
			return GetLeg(m_pairs[pair].leg);
		}
		const Twd::Position & GetLeg(const Pair &pair) const {
			return const_cast<Triangle *>(this)->GetLeg(pair);
		}

		bool IsLegStarted(const Leg &leg) const {
			return m_legs[leg] ? true : false;
		}
		bool IsLegStarted(const Pair &pair) const {
			return IsLegStarted(m_pairs[pair].leg);
		}

		bool IsLegExecuted(const Leg &leg) const {
			return m_legs[leg] && m_legs[leg]->IsOpened();
		}
		bool IsLegExecuted(const Pair &pair) const {
			return IsLegExecuted(m_pairs[pair].leg);
		}

		PairInfo & GetPair(const Pair &pair) {
			return m_pairs[pair];
		}

		const PairInfo & GetPair(const Pair &pair) const {
			return const_cast<Triangle *>(this)->GetPair(pair);
		}

		PairInfo & GetPair(const Leg &leg) {
			return *m_pairsLegs[leg];
		}

		const PairInfo & GetPair(const Leg &leg) const {
			return const_cast<Triangle *>(this)->GetPair(leg);
		}

		const BestBidAskPairs & GetBestBidAsk() const {
			return m_bestBidAsk;
		}

		const TriangulationWithDirection & GetStrategy() const {
			return m_strategy;
		}

	public:

		bool HasOpportunity() const {
			return CheckOpportunity(m_yDirection) == GetY();
		}

		const YDirection & GetYDirection() const {
			return m_yDirection;
		}

		void UpdateYDirection() {
			CalcYDirection(
				IsLegStarted(PAIR_AB)
					?	GetLeg(PAIR_AB).GetSecurity()
					:	m_pairs[PAIR_AB].GetBestSecurity(),
				IsLegStarted(PAIR_BC)
					?	GetLeg(PAIR_BC).GetSecurity()
					:	m_pairs[PAIR_BC].GetBestSecurity(),
				IsLegStarted(PAIR_AC)
					?	GetLeg(PAIR_AC).GetSecurity()
					:	m_pairs[PAIR_AC].GetBestSecurity(),
				m_yDirection);
		}

		void ResetYDirection() {
			Twd::ResetYDirection(m_yDirection);
		}

		double CalcYTargeted() const {

			Assert(m_legs[LEG1] && m_legs[LEG2] && m_legs[LEG3]);
			Assert(
				m_legs[LEG1]->IsOpened()
				&& m_legs[LEG2]->IsOpened()
				&& m_legs[LEG3]->IsOpened());
			Assert(
				!m_legs[LEG1]->IsClosed()
				&& !m_legs[LEG2]->IsClosed()
				&& !m_legs[LEG3]->IsClosed());

			const auto getPrice = [this](const Pair &pair) -> double {
				const Twd::Position &leg = GetLeg(pair);
				const auto &result
					= leg.GetSecurity().DescalePrice(leg.GetOpenStartPrice());
				Assert(!Lib::IsZero(result));
				return result;
			};

			if (m_y ==  Y1) {
				return CalcY1(
					getPrice(PAIR_AB),
					getPrice(PAIR_BC),
					getPrice(PAIR_AC));
			} else {
				AssertEq(Y2, m_y);
				return CalcY2(
					getPrice(PAIR_AB),
					getPrice(PAIR_BC),
					getPrice(PAIR_AC));
			}

		}

		double CalcYExecuted() const {
			
			Assert(m_legs[LEG1] && m_legs[LEG2] && m_legs[LEG3]);
			Assert(
				m_legs[LEG1]->IsOpened()
				&& m_legs[LEG2]->IsOpened()
				&& m_legs[LEG3]->IsOpened());
			Assert(
				!m_legs[LEG1]->IsClosed()
				&& !m_legs[LEG2]->IsClosed()
				&& !m_legs[LEG3]->IsClosed());

			const auto getPrice = [this](const Pair &pair) -> double {
				const Twd::Position &leg = GetLeg(pair);
				const auto &result
					= leg.GetSecurity().DescalePrice(leg.GetOpenPrice());
				Assert(!Lib::IsZero(result));
				return result;
			};

			if (m_y ==  Y1) {
				return CalcY1(
					getPrice(PAIR_AB),
					getPrice(PAIR_BC),
					getPrice(PAIR_AC));
			} else {
				AssertEq(Y2, m_y);
				return CalcY2(
					getPrice(PAIR_AB),
					getPrice(PAIR_BC),
					getPrice(PAIR_AC));
			}
		
		}

	private:

		boost::shared_ptr<Twd::Position> CreateOrder(
				const Leg &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);
		boost::shared_ptr<Twd::Position> CreateOrder(
				const Leg &,
				Security &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);

	private:
		
		TriangulationWithDirection &m_strategy;
		const BestBidAskPairs &m_bestBidAsk;

		TriangleReport m_report;

		const Id m_id;
		const Y m_y;
		const Qty m_qtyStart;
		Qty m_qtyLeg2;
		
		boost::array<PairInfo, numberOfPairs> m_pairs;
		boost::array<PairInfo *, numberOfLegs> m_pairsLegs;
		boost::array<boost::shared_ptr<Twd::Position>, numberOfLegs> m_legs;

		YDirection m_yDirection;
	
	};

} } } }
