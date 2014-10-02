/**************************************************************************
 *   Created: 2014/09/03 00:37:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FxArb1.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace trdk { namespace Strategies { namespace FxMb {
	
	//! Multi-strategy. Each strategy - logic "thread" for equations pair.
	class FxArb1Multi : public FxArb1 {
		
	public:
		
		typedef FxArb1 Base;

	public:

		explicit FxArb1Multi(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Multi", tag, conf) {
			
			// Reading equations pair num from settings for this
			// equations thread:
			const auto equationPairNum
				= conf.ReadTypedKey<size_t>("equations_pair");
			GetLog().Info(
				"Using equations pair number: %1%.",
				equationPairNum);
			if (equationPairNum < 1 || equationPairNum > EQUATIONS_COUNT  / 2) {
				throw Exception("Wrong equations pair number");
			}

			// Storing equations indexes for this "thread":
			m_equations.first = equationPairNum - 1;
			AssertGt(EQUATIONS_COUNT, m_equations.first + EQUATIONS_COUNT / 2);
			m_equations.second = m_equations.first + (EQUATIONS_COUNT / 2);
			AssertNe(m_equations.first, m_equations.second);
				
		}
		
		virtual ~FxArb1Multi() {
			//...//
		}

	public:

	virtual void OnPositionUpdate(Position &positionRef) {

		EquationPosition &position
			= dynamic_cast<EquationPosition &>(positionRef);
		if (!position.IsObservationActive()) {
			return;
		}

		auto &equationPositions
			= GetEquationPosition(position.GetEquationIndex());

		Assert(!position.IsCanceled());

		if (position.IsCompleted()) {
			position.DeactivatieObservation();
			AssertLt(0, equationPositions.activeCount);
			AssertEq(0, equationPositions.waitsForReplyCount);
			if (!--equationPositions.activeCount) {
				position.GetTimeMeasurement().Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
				LogClosingExecuted(position.GetEquationIndex());
				equationPositions.positions.clear();
				if (!CheckCancelAndBlockCondition()) {
					OnOpportunityReturn();
				}
			}
			return;
		}

		if (position.IsOpened() && !position.HasActiveCloseOrders()) {
			AssertLt(0, equationPositions.waitsForReplyCount);
			if (!--equationPositions.waitsForReplyCount) {
				position.GetTimeMeasurement().Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);
				LogOpeningExecuted(position.GetEquationIndex());
			}
			return;
		}

	}
	
	virtual void CheckOpportunity(
					TimeMeasurement::Milestones &timeMeasurement) {

			// Level 1 update callback - will be called every time when
			// ask or bid will be changed for any of configured security:

			CheckConf();

			// Getting more human readable format:
			Broker b1 = GetBroker<1>();
			Broker b2 = GetBroker<2>();
			if (!b1 || !b2) {
				// Not all data received yet (from streams)...
				return;
			}

			const auto &firstEquationPositions
				= GetEquationPosition(m_equations.first);
			const auto &secondEquationPositions
				= GetEquationPosition(m_equations.second);

			if (
					IsInTurnPositionAction(
						firstEquationPositions,
						secondEquationPositions)) {
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return;
			}

			if (firstEquationPositions.activeCount) {
				AssertEq(0, secondEquationPositions.activeCount);
				AssertEq(0, secondEquationPositions.positions.size());
				// We opened on first equation, we try to close on second one
				CheckEquation(
					m_equations.second,
					m_equations.first,
					b1,
					b2,
					timeMeasurement);
				return;
			}
			AssertEq(0, firstEquationPositions.positions.size());

			if (secondEquationPositions.activeCount) {
				// We opened on second equation, we try to close on first one
				CheckEquation(
					m_equations.first,
					m_equations.second,
					b1,
					b2,
					timeMeasurement);
				return;
			}
			AssertEq(0, secondEquationPositions.positions.size());			

			// We haven't opened yet, we try to open

			// Check first equation...
			CheckEquation(
						m_equations.first,
						m_equations.second,
						b1,
						b2,
						timeMeasurement)
				// ... then second if first not "true":
				||	CheckEquation(
						m_equations.second,
						m_equations.first,
						b1,
						b2,
						timeMeasurement);

		}

	private:

		//! Calls and checks equation by index and open positions if equation
		//! returns "true".
		bool CheckEquation(
					size_t equationIndex,
					size_t opposideEquationIndex,
					Broker &b1,
					Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {
			
			AssertNe(equationIndex, opposideEquationIndex);
			AssertEq(0, GetEquationPosition(equationIndex).activeCount);
			AssertEq(0, GetEquationPosition(equationIndex).positions.size());
			
			double equationsResult = .0;
			// Calls equation and exits if it will return "false":
			const auto &equation = GetEquations()[equationIndex];
			b1.equationIndex
				= b2.equationIndex
				= equationIndex;
			if (!equation.first(b1, b2, equationsResult)) {
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return false;
			}
			
			if (GetEquationPosition(opposideEquationIndex).activeCount) {
				AssertLt(
					0,
					GetEquationPosition(opposideEquationIndex)
						.positions.size());
				TurnPositions(
					opposideEquationIndex,
					equationIndex,
					timeMeasurement);
			} else {
				// Opening positions for this equitation:
				OnEquation(
					equationIndex,
					opposideEquationIndex,
					b1,
					b2,
					timeMeasurement);
			}

			return true;

		}

		void OnEquation(
					size_t equationIndex,
					size_t opposideEquationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {

			AssertNe(equationIndex, opposideEquationIndex);
			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());

			// Send open-orders:
			StartPositionsOpening(
				equationIndex,
				opposideEquationIndex,
				b1,
				b2,
				false,
				timeMeasurement);

		}

		void TurnPositions(
					size_t fromEquationIndex,
					size_t toEquationIndex,
					TimeMeasurement::Milestones &timeMeasurement) {

			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_DECISION_START);
			
			auto &fromPositions = GetEquationPosition(fromEquationIndex);
			AssertEq(PAIRS_COUNT, fromPositions.activeCount);
			AssertEq(PAIRS_COUNT, fromPositions.positions.size());
			AssertEq(0, fromPositions.waitsForReplyCount);
			auto &toPositions = GetEquationPosition(toEquationIndex);
			AssertEq(0, toPositions.activeCount);
			AssertEq(0, toPositions.positions.size());
			AssertEq(0, toPositions.waitsForReplyCount);
		
			foreach (auto &fromPosition, fromPositions.positions) {

				boost::shared_ptr<EquationPosition> position;

				if (fromPosition->GetType() == Position::TYPE_LONG) {
					position.reset(
						new EquationShortPosition(
							toEquationIndex,
							fromEquationIndex,
							*this,
							dynamic_cast<EquationLongPosition &>(*fromPosition),
							fromPosition->GetPlanedQty(),
							fromPosition->GetSecurity().GetAskPriceScaled(),
							timeMeasurement));
				} else {
					AssertEq(Position::TYPE_SHORT, fromPosition->GetType());
					position.reset(
						new EquationLongPosition(
							toEquationIndex,
							fromEquationIndex,
							*this,
							dynamic_cast<EquationShortPosition &>(*fromPosition),
							fromPosition->GetPlanedQty(),
							fromPosition->GetSecurity().GetBidPriceScaled(),
							timeMeasurement));
				}

				if (!toPositions.activeCount) {
					timeMeasurement.Measure(
						TimeMeasurement::SM_STRATEGY_EXECUTION_START);
				}

				// Sends orders to broker:
				position->OpenAtMarketPrice();
				
				// Binding all positions into one equation:
				toPositions.positions.push_back(position);
				Verify(++toPositions.activeCount <= PAIRS_COUNT);
				Verify(++toPositions.waitsForReplyCount <= PAIRS_COUNT);

			}

			timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);
			
			toPositions.lastStartTime = boost::get_system_time();
			toPositions.currentOpportunityNumber
				= GetContext().TakeOpportunityNumber();

			LogClosingDetection(fromEquationIndex);
			LogOpeningDetection(toEquationIndex);
				
		}

		bool IsInTurnPositionAction(
					const EquationOpenedPositions &firstEquationPositions,
					const EquationOpenedPositions &secondEquationPositions)
				const {

			if (
					firstEquationPositions.waitsForReplyCount
					|| secondEquationPositions.waitsForReplyCount) {
				return true;
			}

			if (
					firstEquationPositions.activeCount
					&& secondEquationPositions.activeCount) {
				return true;
			}

			const auto &check = [](
						const EquationOpenedPositions &positions)
					-> bool {
				if (!positions.activeCount) {
					AssertEq(0, positions.waitsForReplyCount);
					return false;
				}
				AssertGe(PAIRS_COUNT, positions.activeCount);
				if (positions.activeCount < PAIRS_COUNT) {
					return true;
				}
				foreach (const auto &p, positions.positions) {
					if (p->HasActiveOrders()) {
						return true;
					}
				}
				return false;
			};

			return
				check(firstEquationPositions)
				|| check(secondEquationPositions);

		}

	private:

		void LogOpeningDetection(size_t equationIndex) const {
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(equationIndex).activeCount);
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(equationIndex).waitsForReplyCount);
			LogEquation("Opening detected", equationIndex, false, false);
		}

		void LogOpeningExecuted(size_t equationIndex) const {
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(equationIndex).activeCount);
			AssertEq(0, GetEquationPosition(equationIndex).waitsForReplyCount);
			LogEquation("Opening executed", equationIndex, false, true);
		}

		void LogClosingDetection(size_t equationIndex) const {
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(equationIndex).activeCount);
			AssertEq(0, GetEquationPosition(equationIndex).waitsForReplyCount);
			LogEquation("Closing detected", equationIndex, true, false);
		}

		void LogClosingExecuted(size_t equationIndex) const {
			AssertEq(0, GetEquationPosition(equationIndex).activeCount);
			AssertEq(0, GetEquationPosition(equationIndex).waitsForReplyCount);
			LogEquation("Closing executed", equationIndex, true, true);
		}

		void LogEquation(
					const char *action,
					size_t equationIndex,
					bool isClosing,
					bool isCompleted)
				const {

			const auto &equationPositions = GetEquationPosition(equationIndex);
			const auto &positions = equationPositions.positions;
			AssertEq(PAIRS_COUNT, positions.size());
			if (positions.size() < PAIRS_COUNT) {
				return;
			}

			const auto &p1 = *positions[0];
			const auto &p2 = *positions[1];
			const auto &p3 = *positions[2];

			bool isP1Buy;
			bool isP2Buy;
			bool isP3Buy;

			double p1Price;
			double p2Price;
			double p3Price;

			const auto &getVals = [isClosing, isCompleted](
						const Position &p,
						bool &isBuy,
						double &price) {
				isBuy = p.GetType() == Position::TYPE_LONG
					?	!isClosing
					:	isClosing;
				const auto &scaledPrice = !isCompleted
					?	!isClosing
						?	p.GetOpenStartPrice()
						:	p.GetCloseStartPrice()
					:	!isClosing
						?	p.GetOpenPrice()
						:	p.GetClosePrice();
				AssertNe(0, scaledPrice);
				price = p.GetSecurity().DescalePrice(scaledPrice);
			};

			getVals(p1, isP1Buy, p1Price);
			getVals(p2, isP2Buy, p2Price);
			getVals(p3, isP3Buy, p3Price);

			GetContext().GetLog().Equation(

				equationPositions.currentOpportunityNumber,
		    
				action,
				equationIndex,
		    
				p1.GetTradeSystem().GetTag(),
				p1.GetSecurity().GetSymbol().GetSymbol(),
				p1Price,
				isP1Buy,

				p2.GetTradeSystem().GetTag(),
				p2.GetSecurity().GetSymbol().GetSymbol(),
				p2Price,
				isP2Buy,

				p3.GetTradeSystem().GetTag(),
				p3.GetSecurity().GetSymbol().GetSymbol(),
				p3Price,
				isP3Buy,
		    
				equationIndex < (EQUATIONS_COUNT / 2));

		}

	private:

		std::pair<size_t, size_t> m_equations;

	};
	
} } }


////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API boost::shared_ptr<Strategy> CreateFxArb1MultiStrategy(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf) {
	return boost::shared_ptr<Strategy>(
		new Strategies::FxMb::FxArb1Multi(context, tag, conf));
}

////////////////////////////////////////////////////////////////////////////////

