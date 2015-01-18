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
			m_leg(leg),
			m_isActive(true) {
		}

	public:

		void OpenAtStartPrice() {
			AssertLt(.0, GetOpenStartPrice());
			OpenImmediatelyOrCancel(GetOpenStartPrice());
		}

		virtual void OpenAtCurrentPrice() = 0;

		void CloseAtStartPrice(const CloseType &closeType) {
			Assert(!m_isActive);
			const auto &price = GetOpenStartPrice();
			CloseImmediatelyOrCancel(closeType, price);
			m_isActive = true;
			if (Lib::IsZero(GetCloseStartPrice())) {
				SetCloseStartPrice(price);
			}
		}

		virtual void CloseAtCurrentPrice(const CloseType &closeType) = 0;

		const Pair & GetPair() const {
			return m_pair;
		}

		const Leg & GetLeg() const {
			return m_leg;
		}

		//! @todo remove this workaround: https://trello.com/c/QOBSd8RZ
		bool IsActive() const {
			return m_isActive;
		}

 		//! @todo remove this workaround: https://trello.com/c/QOBSd8RZ
		void Deactivate() {
			Assert(m_isActive);
			m_isActive = false;
		}

	protected:

		void OpenAt(const ScaledPrice &price) {
			if (Lib::IsZero(price)) {
				OpenAtStartPrice();
				return;
			}
			OpenImmediatelyOrCancel(price);
		}

		void CloseAt(const CloseType &closeType, const ScaledPrice &price) {
			if (Lib::IsZero(price)) {
				CloseAtStartPrice(closeType);
				return;
			}
			CloseImmediatelyOrCancel(closeType, price);
			m_isActive = true;
			if (Lib::IsZero(GetCloseStartPrice())) {
				SetCloseStartPrice(price);
			}
		}

	private:

		const Pair m_pair;
		const Leg m_leg;
		bool m_isActive;

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

	public:

		virtual void OpenAtCurrentPrice() {
			AssertLt(.0, GetOpenStartPrice());
			OpenAt(GetSecurity().GetAskPriceScaled());
		}

		virtual void CloseAtCurrentPrice(const CloseType &closeType) {
			Assert(!IsActive());
			CloseAt(closeType, GetSecurity().GetBidPriceScaled());
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

	public:

		void OpenAtCurrentPrice() {
			AssertLt(.0, GetOpenStartPrice());
			OpenAt(GetSecurity().GetBidPriceScaled());
		}

		virtual void CloseAtCurrentPrice(const CloseType &closeType) {
			Assert(!IsActive());
			CloseAt(closeType, GetSecurity().GetAskPriceScaled());
		}

	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
