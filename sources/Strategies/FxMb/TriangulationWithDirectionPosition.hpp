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
#include "Fwd.hpp"
#include "Core/Position.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	////////////////////////////////////////////////////////////////////////////////

	class Position : virtual public trdk::Position {

	public:

		explicit Position(
				const trdk::Qty &qty,
				const Pair &pair,
				const Leg &leg,
				uint8_t qtyPrecision);

	public:

		void Open() {
			AssertLt(.0, GetOpenStartPrice());
			OpenImmediatelyOrCancel(GetOpenStartPrice(), m_orderParams);
		}

		virtual void Close(const CloseType &closeType, double price) {
			Assert(Lib::IsZero(price));
			const auto &scaledPrice = GetSecurity().ScalePrice(price);
			CloseImmediatelyOrCancel(closeType, scaledPrice, m_orderParams);
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
		const OrderParams m_orderParams;

	};

	////////////////////////////////////////////////////////////////////////////////

	class LongPosition : public Twd::Position, public trdk::LongPosition {

	public:

		explicit LongPosition(
				trdk::Strategy &strategy,
				const Triangle &triangle,
				trdk::TradingSystem &tradingSystem,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const Pair &pair,
				const Leg &leg,
				uint8_t qtyPrecision);

		virtual ~LongPosition() {
			//...//
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Twd::Position, public trdk::ShortPosition {

	public:

		explicit ShortPosition(
				trdk::Strategy &strategy,
				const Triangle &triangle,
				trdk::TradingSystem &tradingSystem,
				trdk::Security &security,
				const trdk::Lib::Currency &currency,
				const trdk::Qty &qty,
				const trdk::ScaledPrice &startPrice,
				const Lib::TimeMeasurement::Milestones &timeMeasurement,
				const Pair &pair,
				const Leg &leg,
				uint8_t qtyPrecision);

		~ShortPosition() {
			//...//
		}

	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
