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

#include "TriangulationWithDirectionTypes.hpp"
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
				const Pair &pair,
				const Leg &leg)
			: trdk::Position(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement),
			m_pair(pair),
			m_leg(leg) {
			//...//
		}

	public:

		void Open() {
			AssertLt(.0, GetOpenStartPrice());
			OpenImmediatelyOrCancel(GetOpenStartPrice());
		}

		virtual void Close(const CloseType &closeType, double price) {
			Assert(Lib::IsZero(price));
			const auto &scaledPrice = GetSecurity().ScalePrice(price);
			CloseImmediatelyOrCancel(closeType, scaledPrice);
			if (Lib::IsZero(GetCloseStartPrice())) {
				SetCloseStartPrice(scaledPrice);
			}
		}

		const Pair & GetPair() const {
			return m_pair;
		}

		const Leg & GetLeg() const {
			return m_leg;
		}

	private:

		const Pair m_pair;
		const Leg m_leg;

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
				const Leg &leg)
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
				leg),
			trdk::LongPosition(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement) {
		}

		virtual ~LongPosition() {
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
				const Leg &leg)
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
				leg),
			trdk::ShortPosition(
				strategy,
				tradeSystem,
				security,
				currency,
				qty,
				startPrice,
				timeMeasurement) {
		}

		~ShortPosition() {
			//...//
		}

	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
