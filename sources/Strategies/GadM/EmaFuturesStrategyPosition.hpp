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

#include "Emas.hpp"
#include "Core/Position.hpp"

namespace trdk { namespace Strategies { namespace GadM {
namespace EmaFuturesStrategy {

	////////////////////////////////////////////////////////////////////////////////

	enum Intention {
		INTENTION_OPEN_PASSIVE,
		INTENTION_OPEN_AGGRESIVE,
		INTENTION_HOLD,
		INTENTION_DONOT_OPEN,
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

		explicit Position(
			const Direction &openReason,
			const SlowFastEmas &emas,
			std::ostream &reportStream);
		virtual ~Position();

	public:

		const Time & GetStartTime() const;
		const Time & GetCloseStartTime() const;

		const CloseType & GetOpenType() const;

		const Intention & GetIntention() const;
		
		void SetIntention(
				Intention,
				const CloseType &,
				const Direction &closeReason);

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
		PriceCheckResult CheckTakeProfit(
				double minProfit,
				double trailingPercentage);

		static void OpenReport(std::ostream &);

	protected:
	
		virtual ScaledPrice GetPassiveOpenPrice() const = 0;
		virtual ScaledPrice GetMarketOpenPrice() const = 0;
		virtual ScaledPrice GetPassiveClosePrice() const = 0;
		virtual ScaledPrice GetMarketClosePrice() const = 0;

		virtual ScaledPrice CaclCurrentProfit() const = 0;

		void Report() throw();

	private:

		void Sync(Intention &);

	private:

		Time m_startTime;
		Time m_closeStartTime;
		
		Intention m_intention;
		bool m_isSent;
		bool m_isPassiveOpen;
		bool m_isPassiveClose;

		const SlowFastEmas &m_emas;
		
		CloseType m_closeType;
		boost::array<Direction, 2> m_reasons;
		boost::array<std::pair<double, double>, 2> m_signalsBidAsk;
		boost::array<std::pair<double, double>, 2> m_signalsEmas;

		ScaledPrice m_maxProfit;

		std::ostream &m_reportStream;
	
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
				const Lib::TimeMeasurement::Milestones &,
				const Direction &openReason,
				const SlowFastEmas &emas,
				std::ostream &reportStream);
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
				const Lib::TimeMeasurement::Milestones &,
				const Direction &openReason,
				const SlowFastEmas &emas,
				std::ostream &reportStream);
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
