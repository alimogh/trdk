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
	
	//! Mono-strategy.
	class FxArb1Multi : public FxArb1 {
		
	public:
		
		typedef FxArb1 Base;

	public:

		explicit FxArb1Multi(
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: Base(context, "FxArb1Multi", tag, conf) {
			//...//
		}
		
		virtual ~FxArb1Multi() {
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

		virtual void OnPositionUpdate(trdk::Position &positionRef) {

			// Method calls each time when one of strategy positions updates.
			
			EquationPosition &position
				= dynamic_cast<EquationPosition &>(positionRef);

			if (position.IsCompleted() || position.IsCanceled()) {

				auto &equationPositions
					= GetEquationPosition(position.GetEquationIndex());

				// Position closed, need to find out why:
				if (position.GetOpenedQty() == 0) {
					// IOC was canceled without filling, cancel all another
					// orders at equation and close positions if another orders
					// are filled:
					Assert(!IsEquationOpenedFully(position.GetEquationIndex()));
					CancelAllInEquationAtMarketPrice(
						position.GetEquationIndex(),
						Position::CLOSE_TYPE_OPEN_FAILED);
				}
				AssertEq(0, equationPositions.positions.size());

				// We completed work with this position, forget it...
				Verify(equationPositions.activeCount--);
				return;

			}

			Assert(position.IsStarted());
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(position.GetEquationIndex()).activeCount);
			AssertEq(
				PAIRS_COUNT,
				GetEquationPosition(position.GetEquationIndex()).positions.size());
			Assert(position.IsOpened());
			AssertEq(position.GetPlanedQty(), position.GetOpenedQty());

			if (!IsEquationOpenedFully(position.GetEquationIndex())) {
				// Not all orders are filled yet, we need wait more...
				return;
			}

			position.GetTimeMeasurement().Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY);

			// All equation orders are filled. Now we need to close all
			// positions for opposite equation.
			CancelAllInEquationAtMarketPrice(
				position.GetOppositeEquationIndex(),
				Position::CLOSE_TYPE_TAKE_PROFIT);

		}
		
	protected:

		void OnEquation(
					size_t equationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {

			// Logging current bid/ask values for all pairs
			// (if logging enabled):
			LogBrokersState(equationIndex, b1, b2);

			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_DECISION_START);

			auto &equationPositions = GetEquationPosition(equationIndex);
			AssertEq(0, equationPositions.positions.size());
			AssertEq(0, equationPositions.activeCount);

			// Calculates opposite equation index:
			const auto oppositeEquationIndex
				= equationIndex >= EQUATIONS_COUNT  / 2
					?	equationIndex - (EQUATIONS_COUNT  / 2)
					:	equationIndex + (EQUATIONS_COUNT  / 2);

			AssertEq(BROKERS_COUNT, GetContext().GetTradeSystemsCount());
			const size_t brokerId = equationIndex >= EQUATIONS_COUNT  / 2
				?	2
				:	1;
			const BrokerConf &broker = GetBrokerConf(brokerId);
			TradeSystem &tradeSystem
				= GetContext().GetTradeSystem(brokerId - 1);
			
			// Open new position for each security by equationIndex:
			try {

				foreach (const auto &i, broker.sendList) {
			
					const SecurityPositionConf &conf = i.second;
					// Position must be "shared" as it uses pattern
					// "shared from this":
					boost::shared_ptr<EquationPosition> position;
					if (!conf.isLong) {
						position.reset(
							new EquationShortPosition(
								equationIndex,
								oppositeEquationIndex,
								*this,
								tradeSystem,
								*conf.security,
								conf.security->GetSymbol().GetCashCurrency(),
								conf.qty,
								conf.security->GetBidPriceScaled(),
								timeMeasurement));
					} else {
						position.reset(
							new EquationLongPosition(
								equationIndex,
								oppositeEquationIndex,
								*this,
								tradeSystem,
								*conf.security,
								conf.security->GetSymbol().GetCashCurrency(),
								conf.qty,
								conf.security->GetAskPriceScaled(),
								timeMeasurement));
					}

					if (!equationPositions.activeCount) {
						timeMeasurement.Measure(
							TimeMeasurement::SM_STRATEGY_EXECUTION_START);
					}

					// Sends orders to broker:
					position->OpenAtMarketPriceImmediatelyOrCancel();
				
					// Binding all positions into one equation:
					equationPositions.positions.push_back(position);
					Verify(++equationPositions.activeCount <= PAIRS_COUNT);
			
				}

			} catch (...) {
				CancelAllInEquationAtMarketPrice(
					equationIndex,
					Position::CLOSE_TYPE_SYSTEM_ERROR);
				throw;
			}

			timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);

		}

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

