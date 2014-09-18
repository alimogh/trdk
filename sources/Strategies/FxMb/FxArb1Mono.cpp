/**************************************************************************
 *   Created: 2014/08/15 01:40:01
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
	
	//! Mono-strategy.
	class FxArb1Mono : public FxArb1 {
		
	public:
		
		typedef FxArb1 Base;

	public:

		explicit FxArb1Mono(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Mono", tag, conf) {
			//...//
		}
		
		virtual ~FxArb1Mono() {
			//...//
		}

	public:
		
		virtual void OnLevel1Update(
					Security &,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Level 1 update callback - will be called every time when
			// ask or bid will be changed for any of configured security:

			CheckConf();

			// Getting more human readable format:
			const Broker &b1 = GetBroker(1);
			const Broker &b2 = GetBroker(2);
			if (!b1 || !b2) {
				// Not all data received yet (from streams)...
				return;
			}
			
			double bestEquationsResult = .0;
			size_t bestEquationsIndex = nEquationsIndex;
			// One or more equations has opened positions or active orders.
			size_t currentEquationIndex = nEquationsIndex;
			// Call with actual prices each equation and search for best
			// equation:
			for (size_t i = 0; i < GetEquations().size(); ++i) {
				
				LogBrokersState(i, b1, b2);

				if (GetEquationPosition(i).activeCount) {
					//GetLog().Debug("Equation %1% has active position", (int)i);
					currentEquationIndex = i;
					break;
				}

				// Ask equation for result:
				double currentResult = .0;
				// first - call equation
				const auto &equation = GetEquations()[i];
				
				if (!equation.first(b1, b2, currentResult)) { 
					// Equation not verified.
					continue;
				}
				
				// Check current result for best result:
				if (currentResult > bestEquationsResult) {
					bestEquationsResult = currentResult;
					bestEquationsIndex = i;
				}

			}

			if (currentEquationIndex != nEquationsIndex) {
				// if there is one order opened, we do nothing on opening but
				// we check the closing
				const auto &oppositeEquationIndex
					= GetOppositeEquationIndex(currentEquationIndex);
				// First we should check - is it already has active positions or
				// order or not? At fast updates we can get situation when we
				// already sent orders for this equation at previous MD update,
				// but this orders still not updated own status (opening for
				// this, closing for opposite) as it very fast MD updates, much
				// faster then orders update.
				//GetLog().Debug("Trying to close on equation %1%.", (int)oppositeEquationIndex);

					
				if (!GetEquationPosition(oppositeEquationIndex).activeCount) {
					double currentResult = .0;
					const auto &equation
						= GetEquations()[oppositeEquationIndex];

					LogBrokersState(oppositeEquationIndex, b1, b2);
					
					// first - calls equation
					if (equation.first(b1, b2, currentResult)) {
			
						GetLog().Debug("Going to close orders on equation %1% / 12", (oppositeEquationIndex));

						// Opposite equation verified, we close positions
						/*OnEquation(
							currentEquationIndex,
							false,
							b1,
							b2,
							timeMeasurement);*/
						CancelAllInEquationAtMarketPrice(
						    currentEquationIndex,
						    Position::CLOSE_TYPE_TAKE_PROFIT);
						return;
					}
				}
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return;
			}

			if (bestEquationsIndex == nEquationsIndex) {
				// no one equation with "true" result exists...
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return;
			}

			// "best equation" exists, open positions for it:
			Assert(!Lib::IsZero(bestEquationsResult));
			AssertGt(GetEquations().size(), bestEquationsIndex);

			if (GetEquationPosition(bestEquationsIndex).activeCount) {
				// Equation already has opened positions.
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
				return;
			}
			
			GetLog().Debug("Going to open orders on equation %1% / 12", (bestEquationsIndex + 1));
			
			OnEquation(bestEquationsIndex, true, b1, b2, timeMeasurement);
		
		}

	private:

		void OnEquation(
					size_t equationIndex,
					bool opening,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Calculates opposite equation index:
			const auto &opposideEquationIndex
				= GetOppositeEquationIndex(equationIndex);

			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());

			// Send open-orders:
			StartPositionsOpening(
				equationIndex,
				opposideEquationIndex,
				opening,
				b1,
				b2,
				timeMeasurement);

		}

	};
	
} } }


////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API boost::shared_ptr<Strategy> CreateFxArb1MonoStrategy(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf) {
	return boost::shared_ptr<Strategy>(
		new Strategies::FxMb::FxArb1Mono(context, tag, conf));
}

////////////////////////////////////////////////////////////////////////////////
