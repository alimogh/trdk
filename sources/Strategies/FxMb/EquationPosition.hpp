/**************************************************************************
 *   Created: 2014/08/20 21:44:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Position.hpp"

namespace trdk { namespace Strategies { namespace FxMb {

	////////////////////////////////////////////////////////////////////////////////

	class EquationPosition : virtual public Position {
		
	public:

		explicit EquationPosition(
					size_t equationIndex,
					size_t oppositeEquationIndex,
					trdk::Strategy &strategy,
					trdk::TradeSystem &tradeSystem,
					trdk::Security &security,
					const trdk::Lib::Currency &currency,
					const trdk::Qty &qty,
					const trdk::ScaledPrice &startPrice,
					const Lib::TimeMeasurement::Milestones &timeMeasurement)
				: Position(
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement),
				m_equationIndex(equationIndex),
				m_oppositeEquationIndex(oppositeEquationIndex),
				m_isObservationActive(true) {
			//...//
		}

	public:

		bool IsObservationActive() const {
			return m_isObservationActive;
		}

		void DeactivatieObservation() {
			m_isObservationActive = false;
		}

		size_t GetEquationIndex() const {
			return m_equationIndex;
		}

		size_t GetOppositeEquationIndex() const {
			return m_oppositeEquationIndex;
		}

	private:

		const size_t m_equationIndex;
		const size_t m_oppositeEquationIndex;

		bool m_isObservationActive;

	};

	////////////////////////////////////////////////////////////////////////////////

	class EquationLongPosition
		: public EquationPosition,
		public LongPosition {

	public:

		explicit EquationLongPosition(
					size_t equationIndex,
					size_t oppositeEquationIndex,
					trdk::Strategy &strategy,
					trdk::TradeSystem &tradeSystem,
					trdk::Security &security,
					const trdk::Lib::Currency &currency,
					const trdk::Qty &qty,
					const trdk::ScaledPrice &startPrice,
					const Lib::TimeMeasurement::Milestones &timeMeasurement)
				: Position(
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement),
				EquationPosition(
					equationIndex,
					oppositeEquationIndex,
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement),
				LongPosition(
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement) {
			//...//
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	class EquationShortPosition
		: public EquationPosition,
		public ShortPosition {

	public:

		explicit EquationShortPosition(
					size_t equationIndex,
					size_t oppositeEquationIndex,
					trdk::Strategy &strategy,
					trdk::TradeSystem &tradeSystem,
					trdk::Security &security,
					const trdk::Lib::Currency &currency,
					const trdk::Qty &qty,
					const trdk::ScaledPrice &startPrice,
					const Lib::TimeMeasurement::Milestones &timeMeasurement)
				: Position(
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement),
				EquationPosition(
					equationIndex,
					oppositeEquationIndex,
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement),
				ShortPosition(
					strategy,
					tradeSystem,
					security,
					currency,
					qty,
					startPrice,
					timeMeasurement) {
			//...//
		}

	};

	////////////////////////////////////////////////////////////////////////////////


} } }
