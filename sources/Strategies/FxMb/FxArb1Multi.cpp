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

			CheckConf();

			if (IsInCanceling()) {
				return;
			}

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
					(firstEquationPositions.activeCount
					&& firstEquationPositions.positions.empty())
					|| (secondEquationPositions.activeCount
						&& firstEquationPositions.positions.empty())) {
				// Position in closing process - waiting until it  will be
				// finished:
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
				// Current equation and opposite equation - first closing
				// opposite positions, then wait until any of equations will be
				// "true" again.
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_DECISION_START);
			
				AssertLt(
					0,
					GetEquationPosition(opposideEquationIndex).positions.size());

				// Double amount at closing at this strategy:
				auto &positions = GetEquationPosition(opposideEquationIndex);
				foreach (auto &position, positions.positions) {
					position->SetOpenedQty(position->GetOpenedQty() * 2);
				}

				CancelAllInEquationAtMarketPrice(
					opposideEquationIndex,
					Position::CLOSE_TYPE_TAKE_PROFIT);

				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_DECISION_STOP);
			
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
				timeMeasurement);

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

