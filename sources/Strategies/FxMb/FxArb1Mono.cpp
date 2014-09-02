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
			size_t bestEquationsIndex = std::numeric_limits<size_t>::max();
			// Call with actual prices each equation and search for best
			// equation:
			for (size_t i = 0; i < GetEquations().size(); ++i) {
				// Ask equation for result:
				double currentResult = .0;
				if (!GetEquations()[i](b1, b2, currentResult)) {
					continue;
				}
				// Check current result for best result:
				if (currentResult > bestEquationsResult) {
					bestEquationsResult = currentResult;
					bestEquationsIndex = i;
				}
			}

			if (bestEquationsResult == std::numeric_limits<size_t>::max()) {
				// not one equation with "true" result exists...
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
			OnEquation(bestEquationsIndex, b1, b2, timeMeasurement);
		
		}

	private:

		void OnEquation(
					size_t equationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Calculates opposite equation index:
			const auto opposideEquationIndex
				= equationIndex >= EQUATIONS_COUNT  / 2
					?	equationIndex - (EQUATIONS_COUNT  / 2)
					:	equationIndex + (EQUATIONS_COUNT  / 2);

			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());
			// if 0 - 1 equations sends orders to first broker,
			// if 6 - 11 - to second broker:
			const size_t brokerId = equationIndex >= EQUATIONS_COUNT  / 2
				?	2
				:	1;

			// Send open-orders:
			StartPositionsOpening(
				equationIndex,
				opposideEquationIndex,
				brokerId,
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
