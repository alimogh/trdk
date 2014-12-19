/**************************************************************************
 *   Created: 2014/12/15 14:06:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Position.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	////////////////////////////////////////////////////////////////////////////////

	class Position : virtual public trdk::Position {
		
	public:

		explicit Position(
				trdk::Strategy &strategy,
				trdk::TradeSystem &tradeSystem,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const size_t leg,
				bool isByRising)
			: trdk::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement),
			m_leg(leg),
			m_isByRising(isByRising),
			m_isCompleted(false) {
			Assert(!Lib::IsZero(startPrice));
		}

	public:

		void Open() {
			OpenImmediatelyOrCancel(GetOpenStartPrice());
			// OpenAtMarketPrice();
		}

		size_t GetLeg() const {
			return m_leg;
		}

		bool IsByRising() const {
			return m_isByRising;
		}

		//! @todo remove this workaround
		bool IsCompleted() const {
			return m_isCompleted;
		}

		//! @todo remove this workaround
		void Complete() {
			Assert(!m_isCompleted);
			m_isCompleted = true;
		}

	private:

		const size_t m_leg;
		const bool m_isByRising;
		bool m_isCompleted;

	};

	////////////////////////////////////////////////////////////////////////////////

	class LongPosition : public Twd::Position, public trdk::LongPosition {

	public:

		explicit LongPosition(
				trdk::Strategy &strategy,
				trdk::TradeSystem &tradeSystem,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const size_t leg,
				bool isByRising)
			: trdk::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement),
			Twd::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement,
				leg,
				isByRising),
			trdk::LongPosition(
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

	class ShortPosition : public Twd::Position, public trdk::ShortPosition {

	public:

		explicit ShortPosition(
					trdk::Strategy &strategy,
					trdk::TradeSystem &tradeSystem,
					trdk::Security &security,
					const trdk::Lib::Currency &currency,
					const trdk::Qty &qty,
					const trdk::ScaledPrice &startPrice,
					const Lib::TimeMeasurement::Milestones &timeMeasurement,
					const size_t leg,
					bool isByRising)
			: trdk::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement),
			Twd::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement,
				leg,
				isByRising),
			trdk::ShortPosition(
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

} } } }
