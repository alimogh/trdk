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

	const char * ConvertToPch(const Intention &);

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

		const boost::posix_time::ptime & GetStartTime() const;
		const boost::posix_time::ptime & GetCloseStartTime() const;

		const CloseType & GetOpenType() const;

		const Direction & GetOpenReason() const;

		const Intention & GetIntention() const;
		
		void SetIntention(
				Intention,
				const CloseType &,
				const Direction &closeReason);
		void SetIntention(
				Intention,
				const CloseType &,
				const Direction &closeReason,
				const Qty &intentionSize);

		void MoveOrderToCurrentPrice();

		void Sync();

		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const = 0;
		virtual PriceCheckResult CheckStopLoss(
				double maxLossMoneyPerContract)
				const
				= 0;
		PriceCheckResult CheckTakeProfit(
				double minProfit,
				double trailingRatio)
				const;
		//! Checks closing by trailing stop.
		/** @sa https://app.asana.com/0/186349222941752/196887742980415
		  */
		PriceCheckResult CheckTrailingStop(const TrailingStop &) const;

		//! Checks profit level.
		/** @sa https://app.asana.com/0/196887491555385/192879506137993
		  */
		Qty CheckProfitLevel(const ProfitLevels &) const;

		static void OpenReport(std::ostream &);

	protected:

		void Report() noexcept;

	private:

		void Sync(Intention &);

	private:

		boost::posix_time::ptime m_startTime;
		boost::posix_time::ptime m_closeStartTime;
		
		Intention m_intention;
		boost::optional<std::pair<Qty, Qty>> m_intentionSize;
		bool m_isSent;
		bool m_isPassiveOpen;
		bool m_isPassiveClose;

		const SlowFastEmas &m_emas;
		
		CloseType m_closeType;
		boost::array<Direction, 2> m_reasons;
		boost::array<std::pair<double, double>, 2> m_signalsBidAsk;
		boost::array<std::pair<double, double>, 2> m_signalsEmas;

		ScaledPrice m_maxProfitTakeProfit;
		ScaledPrice m_maxProfitTrailingStop;

		std::ostream &m_reportStream;
	
	};

	////////////////////////////////////////////////////////////////////////////////

	class LongPosition : public Position, public trdk::LongPosition {
	public:
		explicit LongPosition(
				Strategy &,
				const boost::uuids::uuid &,
				TradingSystem &,
				Security &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &,
				const Direction &openReason,
				const SlowFastEmas &emas,
				std::ostream &reportStream);
		virtual ~LongPosition();
	public:
		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLoss(
				double maxLossMoneyPerContract)
				const;
	};

	////////////////////////////////////////////////////////////////////////////////

	class ShortPosition : public Position, public trdk::ShortPosition {
	public:
		explicit ShortPosition(
				Strategy &,
				const boost::uuids::uuid &,
				TradingSystem &,
				Security &,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &,
				const Direction &openReason,
				const SlowFastEmas &emas,
				std::ostream &reportStream);
		virtual ~ShortPosition();
	public:
		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const;
		virtual PriceCheckResult CheckStopLoss(
				double maxLossMoneyPerContract)
				const;
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
