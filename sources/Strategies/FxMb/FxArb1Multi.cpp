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
			
			if (GetEquationPosition(m_equations.first).activeCount)
			{
				// We opened on first equation, we try to close on second one
				CheckEquation(
					m_equations.second,
					m_equations.first,
					b1,
					b2,
					timeMeasurement);
			}
			else if (GetEquationPosition(m_equations.second).activeCount)
			{
				// We opened on second equation, we try to close on first one
				CheckEquation(m_equations.first,
					m_equations.second,
					b1,
					b2,
					timeMeasurement);
			}
			else
			{
				// We haven't opened yet, we try to open

				// Call threads equations:
				if (	CheckEquation(m_equations.first,
							m_equations.second,
							b1,
							b2,
							timeMeasurement)) {
					// First equation returns "true", so we sent orders for it and
					// exit.
					return;
				}

				// First equation returns "false", so we check opposide equation:
				CheckEquation(
					m_equations.second,
					m_equations.first,
					b1,
					b2,
					timeMeasurement);
			}
		}

	private:

		//! Calls and checks equation by index and open positions if equation
		//! returns "true".
		bool CheckEquation(
					size_t equationIndex,
					size_t opposideEquationIndex,
					const Broker &b1,
					const Broker &b2,
					TimeMeasurement::Milestones &timeMeasurement) {
			
			AssertNe(equationIndex, opposideEquationIndex);

			if (GetEquationPosition(equationIndex).activeCount) {
				// This equation already has opened positions or orders.
				return false;
			}
			
			double equationsResult = .0;
			// Calls equation and exits if it will return "false":
			const auto &equation = GetEquations()[equationIndex];
			if (!equation.first(b1, b2, equationsResult)) {
				return false;
			}
			
			// Opening positions for this equitation:
			OnEquation(
				equationIndex,
				opposideEquationIndex,
				b1,
				b2,
				timeMeasurement);
			
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

