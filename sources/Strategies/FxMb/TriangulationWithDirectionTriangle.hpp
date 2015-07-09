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
			//! Logic direction, ie we need to close position by logic.
			/** @sa PairLegParams::isBuyForOrder
			  */
			bool isBuy;
			size_t ecn;
		};

		struct PairInfo : public PairLegParams {

			//! Real direction, ie we should reverse logic direction by quote
			//! currency.
			/** @sa PairLegParams::isBuy
			  */
			bool isBuyForOrder;

			const BestBidAsk *bestBidAsk;
			Security *security;

			double startPrice;
			size_t ordersCount;

			PairInfo() {
				//...//
			}

			explicit PairInfo(
					const PairLegParams &params,
					const BestBidAskPairs &bestBidAskRef)
				: PairLegParams(params),
				isBuyForOrder(isBuy),
				bestBidAsk(&bestBidAskRef[id]),
				security(
					//! @todo FIXME const_cast for security 
					const_cast<Security *>(
						&bestBidAsk->service->GetSecurity(ecn))),
				startPrice(GetCurrentPrice()),
				ordersCount(0) {
				if (Lib::IsZero(startPrice)) {
					throw HasNotMuchOpportunityException(*security, 0);
				}
			}

			double GetCurrentPrice() const {
				return isBuy
					?	security->GetAskPrice()
					:	security->GetBidPrice();
			}

			double GetCurrentBestPrice() const {
				const Security &security = GetBestSecurity();
				return isBuy
					?	security.GetAskPrice()
					:	security.GetBidPrice();
			}

			Security & GetBestSecurity() {
				//! @todo FIXME const_cast for security 
				return const_cast<Security &>(
					bestBidAsk->service->GetSecurity(
						isBuy
							?	bestBidAsk->bestAsk.source
							:	bestBidAsk->bestBid.source));
			}

			const Security & GetBestSecurity() const {
				return const_cast<PairInfo *>(this)->GetBestSecurity();
			}

		};

	public:

		explicit Triangle(
				const Id &,
				TriangulationWithDirection &,
				ReportsState &,
				const Y &,
				const Qty &startQty,
				const PairLegParams &ab,
				const PairLegParams &bc,
				const PairLegParams &ac,
				const BestBidAskPairs &bestBidAskRef,
				size_t bookUpdatesNumber);
		~Triangle();

	public:

		void StartLeg1(
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const PairsSpeed &pairsSpeed) {
		
			Assert(!IsLegStarted(LEG1));
			Assert(!IsLegStarted(LEG2));
			Assert(!IsLegStarted(LEG3)); 
			if (
					IsLegStarted(LEG1)
					|| IsLegStarted(LEG2)
					|| IsLegStarted(LEG3)) {
				throw Lib::LogicError(
					"Failed to start triangle leg 1 (wrong strategy logic)");
			}

			PairInfo &pair = GetPair(LEG1);

			boost::shared_ptr<Twd::Position> order = CreateOrder(
				pair,
				*pair.security,
				pair.startPrice,
				timeMeasurement);

			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_START_1);
			order->Open();
			const auto &orderDelay = timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_COMPLETE_1);

			m_legs[LEG1] = order;

			m_report.ReportAction(
				"detected",
				"signal",
				"1",
				orderDelay,
				nullptr,
				&pairsSpeed);

		}

		void StartLeg2() {

			Assert(IsLegStarted(LEG1));
			Assert(!IsLegStarted(LEG2));
			Assert(!IsLegStarted(LEG3));

			Assert(GetLeg(LEG1).IsOpened());
			Assert(!GetLeg(LEG1).IsClosed());

			if (
					!IsLegStarted(LEG1)
					|| !GetLeg(LEG1).IsOpened()
					|| GetLeg(LEG1).IsClosed()
					|| IsLegStarted(LEG2)
					|| IsLegStarted(LEG3)) {
				throw Lib::LogicError(
					"Failed to start triangle leg 2 (wrong strategy logic)");
			}
			
			const auto &price = GetPair(LEG2).GetCurrentPrice();
			if (Lib::IsZero(price)) {
				throw HasNotMuchOpportunityException(
					*GetPair(LEG2).security,
					0);
			}

			boost::shared_ptr<Twd::Position> order = CreateOrder(
				GetPair(LEG2),
				GetPair(LEG2).GetBestSecurity(),
				price,
				Lib::TimeMeasurement::Milestones());
			order->Open();

			m_legs[LEG2] = order;

		}

		Lib::TimeMeasurement::PeriodFromStart StartLeg3(
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				bool isResend) {

			Assert(IsLegStarted(LEG1));
			Assert(IsLegStarted(LEG2));
			Assert(!IsLegStarted(LEG3));

			Assert(GetLeg(LEG1).IsOpened());
			Assert(!GetLeg(LEG1).IsClosed());

			Assert(GetLeg(LEG2).IsOpened());
			Assert(!GetLeg(LEG2).IsClosed());

			if (
					!IsLegStarted(LEG1)
					|| !GetLeg(LEG1).IsOpened()
					|| GetLeg(LEG1).IsClosed()
					|| !IsLegStarted(LEG2)
					|| !GetLeg(LEG2).IsOpened()
					|| GetLeg(LEG2).IsClosed()
					|| IsLegStarted(LEG3)) {
				throw Lib::LogicError(
					"Failed to start triangle leg 3 (wrong strategy logic)");
			}

			const auto &price = GetPair(LEG3).GetCurrentPrice();
			if (Lib::IsZero(price)) {
				throw HasNotMuchOpportunityException(
					*GetPair(LEG3).security,
					0);
			}

			const boost::shared_ptr<Twd::Position> order = CreateOrder(
				GetPair(LEG3),
				GetPair(LEG3).GetBestSecurity(),
				price,
				timeMeasurement);

			if (!isResend) {
				timeMeasurement.Measure(
					Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_START_2);
			}
			order->Open();
			Lib::TimeMeasurement::PeriodFromStart orderDelay = 0;
			if (!isResend) {
				orderDelay = timeMeasurement.Measure(
					Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_COMPLETE_2);
			}

			m_legs[LEG3] = order;

			return orderDelay;

		}

		void Cancel(const Position::CloseType &closeType) {
			
			Assert(IsLegStarted(LEG1));
			Assert(!IsLegStarted(LEG2));
			Assert(!IsLegStarted(LEG3));

			Assert(!GetLeg(LEG1).HasActiveOrders());

			if (
					!IsLegStarted(LEG1)
					|| IsLegStarted(LEG2)
					|| IsLegStarted(LEG3)) {
				throw Lib::LogicError(
					"Failed to cancel triangle (wrong strategy logic)");
			}

			GetLeg(LEG1).Close(closeType, GetPair(LEG1).GetCurrentPrice());

		}

	public:

		void OnLeg2Cancel() {
			
			Assert(IsLegStarted(LEG1));
			Assert(IsLegStarted(LEG2));
			Assert(!IsLegStarted(LEG3));

			Assert(GetLeg(LEG1).IsOpened());
			Assert(!GetLeg(LEG2).IsOpened());

			m_legs[LEG2].reset();

		}

		void OnLeg3Cancel() {
			
			Assert(IsLegStarted(LEG1));
			Assert(IsLegStarted(LEG2));
			Assert(IsLegStarted(LEG3));

			Assert(GetLeg(LEG1).IsOpened());
			Assert(GetLeg(LEG2).IsOpened());
			Assert(!GetLeg(LEG3).IsOpened());

			m_legs[LEG3].reset();
		
		}

	public:

		const Id & GetId() const {
			return m_id;
		}

		const Y & GetY() const {
			return m_y;
		}

		double GetLastAtr() const {
			return .0;
		}

		size_t GetBookUpdatesNumber() const {
			return m_bookUpdatesNumber;
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

		bool HasActiveOrders() const {
			foreach (const auto &leg, m_legs) {
				if (leg && !leg->HasActiveOrders()) {
					return false;
				}
			}
			return true;
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

		const Security & GetCalcSecurity(const Pair &pair) const {
			return IsLegExecuted(pair)
				?	GetLeg(pair).GetSecurity()
				:	*GetPair(pair).security;
		}

		const TriangulationWithDirection & GetStrategy() const {
			return m_strategy;
		}

		const boost::posix_time::ptime & GetStartTime() const {
			return m_startTime;
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
				GetCalcSecurity(PAIR_AB),
				GetCalcSecurity(PAIR_BC),
				GetCalcSecurity(PAIR_AC),
				m_yDirection);
		}

		void ResetYDirection() {
			Twd::ResetYDirection(m_yDirection);
		}

		double CalcYTargeted() const {

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
				PairInfo &,
				Security &,
				double price,
				const Lib::TimeMeasurement::Milestones &);

	private:
		
		TriangulationWithDirection &m_strategy;
		const boost::posix_time::ptime m_startTime;
		const BestBidAskPairs &m_bestBidAsk;

		TriangleReport m_report;

		const Id m_id;
		const Y m_y;
		const Qty m_aQty;

		boost::array<PairInfo, numberOfPairs> m_pairs;
		boost::array<PairInfo *, numberOfLegs> m_pairsLegs;
		boost::array<boost::shared_ptr<Twd::Position>, numberOfLegs> m_legs;

		YDirection m_yDirection;

		const size_t m_bookUpdatesNumber;
	
	};

} } } }
