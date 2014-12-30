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

	enum Pair {
		//! Like a EUR/USD.
		PAIR_AB,
		//! Like a USD/JPY.
		PAIR_BC,
		//! Like a EUR/JPY.
		PAIR_AC,
		numberOfPairs = 3
	};

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
				const Pair &pair,
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
			m_pair(pair),
			m_leg(leg),
			m_isByRising(isByRising),
			m_isCompleted(false),
			m_closeStep(0) {
			Assert(!Lib::IsZero(startPrice));
		}

	public:

		void Open() {
			OpenImmediatelyOrCancel(GetOpenStartPrice());
		}

		size_t ResetCloseSteps() {
			const auto closeStep = m_closeStep;
			m_closeStep = 0;
			return closeStep;
		}

		size_t TakeCloseStep() {
			return m_closeStep++;
		}

		const Pair & GetPair() const {
			return m_pair;
		}

		size_t GetLeg() const {
			return m_leg;
		}

		bool IsByRising() const {
			return m_isByRising;
		}

		//! @todo remove this workaround: https://trello.com/c/QOBSd8RZ
		bool IsCompleted() const {
			return m_isCompleted;
		}

		//! @todo remove this workaround: https://trello.com/c/QOBSd8RZ
		void Complete() {
			Assert(!m_isCompleted);
			m_isCompleted = true;
		}

		//! @todo remove this workaround: https://trello.com/c/QOBSd8RZ
		void Uncomplete() {
			Assert(m_isCompleted);
			m_isCompleted = false;
		}

	private:

		const Pair m_pair;
		const size_t m_leg;
		const bool m_isByRising;
		bool m_isCompleted;
		size_t m_closeStep;

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
				const Pair &pair,
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
				pair,
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
				const Pair &pair,
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
				pair,
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
