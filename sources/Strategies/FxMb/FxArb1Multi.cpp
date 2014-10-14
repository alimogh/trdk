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
	
		virtual void CheckOpportunity(
					TimeMeasurement::Milestones &timeMeasurement) {

			// Level 1 update callback - will be called every time when
			// ask or bid will be changed for any of configured security:

			// Getting more human readable format:
			Broker b1 = GetBroker<1>();
			Broker b2 = GetBroker<2>();
			if (!b1 || !b2) {
				// Not all data received yet (from streams)...
				return;
			}

			const auto &firstEquationPositions
				= GetEquationPositions(m_equations.first);
			const auto &secondEquationPositions
				= GetEquationPositions(m_equations.second);

			if (
					IsInTurnPositionAction(
						firstEquationPositions,
						secondEquationPositions)) {
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return;
			}

			if (firstEquationPositions.activeCount) {
				if (IsInGracePeriod(firstEquationPositions)) {
					return;
				}
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
				if (IsInGracePeriod(secondEquationPositions)) {
					return;
				}
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

		virtual void CloseDelayed(
					size_t equationIndex,
					TimeMeasurement::Milestones &timeMeasurement) {
			auto &positions = GetEquationPositions(equationIndex);
			const auto &oppositeEquationIndex
				= GetOppositeEquationIndex(equationIndex);
			AssertEq(
				0,
				GetEquationPositions(oppositeEquationIndex).waitsForReplyCount);
			foreach (auto &position, positions.positions) {
				if (position->IsCompleted()) {
					continue;
				}
				AssertGt(
					PAIRS_COUNT,
					GetEquationPositions(oppositeEquationIndex).activeCount);
				AssertGt(
					PAIRS_COUNT,
					GetEquationPositions(oppositeEquationIndex).waitsForReplyCount);
				AssertGt(
					PAIRS_COUNT,
					GetEquationPositions(oppositeEquationIndex).positions.size());
				TurnPosition(
					*position,
					positions,
					oppositeEquationIndex,
					timeMeasurement);
			}
		}

		virtual bool OnCanceling() {
			size_t ordersSent = 0;
			for (size_t i = 0; i < EQUATIONS_COUNT; ++i) {
				ordersSent += CancelEquation(
					i,
					Position::CLOSE_TYPE_ENGINE_STOP);
			}
			AssertGe(PAIRS_COUNT, ordersSent);
			return ordersSent == 0;
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
			AssertEq(0, GetEquationPositions(equationIndex).activeCount);
			AssertEq(0, GetEquationPositions(equationIndex).positions.size());
			
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
			
			if (GetEquationPositions(opposideEquationIndex).activeCount) {
				AssertLt(
					0,
					GetEquationPositions(opposideEquationIndex)
						.positions.size());
				LogBrokersState(equationIndex, b1, b2);
				TurnPositions(
					opposideEquationIndex,
					equationIndex,
					timeMeasurement);
			} else {
				// Opening positions for this equitation:
				OnFirstEquation(
					equationIndex,
					opposideEquationIndex,
					b1,
					b2,
					timeMeasurement);
			}

			return true;

		}

		void OnFirstEquation(
					size_t equationIndex,
					size_t opposideEquationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {
			AssertNe(equationIndex, opposideEquationIndex);
			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());
			StartPositionsOpening(
				equationIndex,
				opposideEquationIndex,
				b1,
				b2,
				timeMeasurement);
		}

		void TurnPositions(
					size_t fromEquationIndex,
					size_t toEquationIndex,
					TimeMeasurement::Milestones &timeMeasurement) {

			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_DECISION_START);
			
			auto &fromPositions = GetEquationPositions(fromEquationIndex);
			AssertEq(PAIRS_COUNT, fromPositions.activeCount);
			AssertEq(PAIRS_COUNT, fromPositions.positions.size());
			AssertEq(0, fromPositions.waitsForReplyCount);

			auto &toPositions = GetEquationPositions(toEquationIndex);
			Lib::UseUnused(toPositions);
			AssertEq(0, toPositions.activeCount);
			AssertEq(0, toPositions.positions.size());
			AssertEq(0, toPositions.waitsForReplyCount);
		
			foreach (auto &fromPosition, fromPositions.positions) {
				TurnPosition(
					*fromPosition,
					fromPositions,
					toEquationIndex,
					timeMeasurement);
			}

			timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);

			toPositions.lastStartTime = boost::get_system_time();
			toPositions.currentOpportunityNumber
				= GetContext().TakeOpportunityNumber();

			LogClosingDetection(fromEquationIndex);
			LogOpeningDetection(toEquationIndex);
			
		}

		
		void TurnPosition(
					EquationPosition &fromPosition,
					EquationOpenedPositions &fromPositions,
					size_t toEquationIndex,
					TimeMeasurement::Milestones &timeMeasurement) {

			auto &toPositions = GetEquationPositions(toEquationIndex);
			AssertGt(PAIRS_COUNT, toPositions.activeCount);
			AssertGt(PAIRS_COUNT, toPositions.waitsForReplyCount);
			AssertGt(PAIRS_COUNT, toPositions.positions.size());

			boost::shared_ptr<EquationPosition> position;

			if (fromPosition.GetType() == Position::TYPE_LONG) {
				position.reset(
					new EquationShortPosition(
						toEquationIndex,
						fromPosition.GetEquationIndex(),
						*this,
						dynamic_cast<EquationLongPosition &>(fromPosition),
						fromPosition.GetPlanedQty(),
						fromPosition.GetSecurity().GetAskPriceScaled(),
						timeMeasurement));
			} else {
				AssertEq(Position::TYPE_SHORT, fromPosition.GetType());
				position.reset(
					new EquationLongPosition(
						toEquationIndex,
						fromPosition.GetEquationIndex(),
						*this,
						dynamic_cast<EquationShortPosition &>(fromPosition),
						fromPosition.GetPlanedQty(),
						fromPosition.GetSecurity().GetBidPriceScaled(),
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
			
			AssertGe(PAIRS_COUNT, toPositions.activeCount);
			++toPositions.activeCount;
			AssertGe(PAIRS_COUNT, fromPositions.waitsForReplyCount);
			++fromPositions.waitsForReplyCount;
			AssertGe(PAIRS_COUNT, toPositions.waitsForReplyCount);
			++toPositions.waitsForReplyCount;

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

