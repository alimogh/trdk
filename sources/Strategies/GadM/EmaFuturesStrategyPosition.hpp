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
			double start;
			double margin;
			double current;
		};

	public:

		Position();
		virtual ~Position();

	public:

		const Time & GetStartTime() const;

		const Intention & GetIntention() const;
		
		void SetIntention(Intention);

		void MoveOrderToCurrentPrice();

		void Sync();

		virtual PriceCheckResult CheckOrderPrice(double priceDelta) const = 0;

	protected:
	
		virtual double GetPassiveOpenPrice() const = 0;
		virtual double GetMarketOpenPrice() const = 0;
		virtual double GetPassiveClosePrice() const = 0;
		virtual double GetMarketClosePrice() const = 0;
		

	private:

		void Sync(Intention &);

	private:

		Time m_startTime;
		
		Intention m_intention;
		bool m_isSent;
		bool m_isPassiveOpen;
		bool m_isPassiveClose;
	
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
	protected:
		virtual double GetPassiveOpenPrice() const;
		virtual double GetMarketOpenPrice() const;
		virtual double GetPassiveClosePrice() const;
		virtual double GetMarketClosePrice() const;
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
	protected:
		virtual double GetPassiveOpenPrice() const;
		virtual double GetMarketOpenPrice() const;
		virtual double GetPassiveClosePrice() const;
		virtual double GetMarketClosePrice() const;
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
