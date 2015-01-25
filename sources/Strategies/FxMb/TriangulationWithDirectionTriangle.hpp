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
			size_t ecn;
		};

		struct PairInfo : public PairLegParams {
			
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
				bestBidAsk(&bestBidAskRef[id]),
				security(
					//! @todo FIXME const_cast for security 
					const_cast<Security *>(
						&bestBidAsk->service->GetSecurity(ecn))),
				startPrice(GetCurrentPrice()),
				ordersCount(0) {
			}

			double GetCurrentPrice() const {
				return isBuy
					?	security->GetAskPrice()
					:	security->GetBidPrice();
			}

			Security & GetBestSecurity() {
				//! @todo FIXME const_cast for security 
				return const_cast<Security &>(
					bestBidAsk->service->GetSecurity(
						isBuy
							?	bestBidAsk->bestAsk.source
							:	bestBidAsk->bestBid.source));
			}

		};

	public:

		explicit Triangle(
				const Id &id,
				TriangulationWithDirection &strategy,
				ReportsState &reportsState,
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

#			endif

			UpdateYDirection();

		}

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

			if (!m_qtyLeg2) {

				const auto &leg1Security = GetLeg(LEG1).GetSecurity();
				const double leg1Price
					= leg1Security.DescalePrice(GetLeg(LEG1).GetOpenPrice());
				const auto leg1Vol = leg1Price * GetLeg(LEG1).GetOpenedQty();

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

			PairInfo &pair = GetPair(LEG2);
			boost::shared_ptr<Twd::Position> order = CreateOrder(
				pair,
				*pair.security,
				GetPair(LEG2).GetCurrentPrice(),
				m_qtyLeg2,
				Lib::TimeMeasurement::Milestones());
			order->OpenAtStartPrice();

			m_legs[LEG2] = order;

		}

		void StartLeg3(
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {

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

			const double leg1Price = GetLeg(LEG1).GetSecurity().DescalePrice(
				GetLeg(LEG1).GetOpenPrice());
			const auto leg1Vol = leg1Price * GetLeg(LEG1).GetOpenedQty();

			PairInfo &leg3Pair = GetPair(LEG3);
			const boost::shared_ptr<Twd::Position> order = CreateOrder(
				leg3Pair,
				leg3Pair.GetBestSecurity(),
				leg3Pair.GetCurrentPrice(),
				//! @todo remove "to qty"
				Qty(leg1Vol),
				timeMeasurement);

			timeMeasurement.Measure(
				Lib::TimeMeasurement::SM_STRATEGY_EXECUTION_START);

			order->OpenAtStartPrice();
			m_legs[LEG3] = order;

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

			GetLeg(LEG1).CloseAtCurrentPrice(closeType);

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

		const Security & GetCalcSecurity(const Pair &pair) const {
			return IsLegExecuted(pair)
				?	GetLeg(pair).GetSecurity()
				:	*GetPair(pair).security;
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
				GetCalcSecurity(PAIR_AB),
				GetCalcSecurity(PAIR_BC),
				GetCalcSecurity(PAIR_AC),
				m_yDirection);
		}

		void ResetYDirection() {
			Twd::ResetYDirection(m_yDirection);
		}

		double CalcYTargeted() const {

			Assert(IsLegStarted(LEG1));
			Assert(IsLegStarted(LEG2));
			Assert(IsLegStarted(LEG3));
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
				Security &security,
				double price,
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
