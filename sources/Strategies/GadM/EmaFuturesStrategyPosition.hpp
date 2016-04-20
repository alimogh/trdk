/**************************************************************************
 *   Created: 2016/04/13 22:09:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Position.hpp"

namespace trdk { namespace Strategies { namespace GadM {
namespace EmaFuturesStrategy {

	////////////////////////////////////////////////////////////////////////////////

	enum Intention {
		INTENTION_OPEN_PASSIVE,
		INTENTION_DONOT_OPEN,
		INTENTION_OPEN_AGGRESIVE,
		INTENTION_HOLD,
		INTENTION_CLOSE_PASSIVE,
		INTENTION_CLOSE_AGGRESIVE,
		numberOfIntentions
	};

	////////////////////////////////////////////////////////////////////////////////

	class Position : virtual public trdk::Position {

	public:

		typedef trdk::Position Base;

		struct PriceCheckResult {
			bool isAllowed;
			ScaledPrice start;
			ScaledPrice margin;
			ScaledPrice current;
		};

	public:

		Position();
		virtual ~Position();

	public:

		const Time & GetStartTime() const;

		const Intention & GetIntention() const;
		
		void SetIntention(Intention, const CloseType &);

		void MoveOrderToCurrentPrice();

		void Sync();

		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const = 0;
		virtual PriceCheckResult CheckStopLossByPrice(
				double priceDelta)
				const
				= 0;
		virtual PriceCheckResult CheckStopLossByLoss(
				double maxLossMoneyPerContract)
				const
				= 0;
		PriceCheckResult CheckTakeProfit(double trailingPercentage);

	protected:
	
		virtual ScaledPrice GetPassiveOpenPrice() const = 0;
		virtual ScaledPrice GetMarketOpenPrice() const = 0;
		virtual ScaledPrice GetPassiveClosePrice() const = 0;
		virtual ScaledPrice GetMarketClosePrice() const = 0;

		virtual ScaledPrice CaclCurrentProfit() const = 0;
		

	private:

		void Sync(Intention &);

	private:

		Time m_startTime;
		
		Intention m_intention;
		bool m_isSent;
		bool m_isPassiveOpen;
		bool m_isPassiveClose;

		CloseType m_closeType;

		ScaledPrice m_maxProfit;
	
	};

	////////////////////////////////////////////////////////////////////////////////

	class LongPosition : public Position, public trdk::LongPosition {
	public:
		explicit LongPosition(
				Strategy &,
				const boost::uuids::uuid &,
				TradeSystem &,
				Security &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);
		virtual ~LongPosition();
	public:
		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLossByPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLossByLoss(
				double maxLossMoneyPerContract)
				const;
	protected:
		virtual ScaledPrice GetPassiveOpenPrice() const;
		virtual ScaledPrice GetMarketOpenPrice() const;
		virtual ScaledPrice GetPassiveClosePrice() const;
		virtual ScaledPrice GetMarketClosePrice() const;
		virtual ScaledPrice CaclCurrentProfit() const;
	};

	////////////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Position, public trdk::ShortPosition {
	public:
		explicit ShortPosition(
				Strategy &,
				const boost::uuids::uuid &,
				TradeSystem &,
				Security &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);
		virtual ~ShortPosition();
	public:
		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLossByPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLossByLoss(
				double maxLossMoneyPerContract)
				const;
	protected:
		virtual ScaledPrice GetPassiveOpenPrice() const;
		virtual ScaledPrice GetMarketOpenPrice() const;
		virtual ScaledPrice GetPassiveClosePrice() const;
		virtual ScaledPrice GetMarketClosePrice() const;
		virtual ScaledPrice CaclCurrentProfit() const;
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
