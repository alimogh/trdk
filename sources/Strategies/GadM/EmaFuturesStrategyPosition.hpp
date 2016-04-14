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

	public:

		Position();
		virtual ~Position();

	public:

		const Time & GetStartTime() const;

		const Intention & GetIntention() const;
		
		void SetIntention(Intention);

		void Sync();

	protected:

		virtual ScaledPrice GetMarketOpenPrice() const = 0;
		virtual ScaledPrice GetMarketClosePrice() const = 0;

	private:

	
		void Sync(Intention &, bool &);


	private:

		const Time m_startTime;
		
		Intention m_intention;
		bool m_isIntentionInAction;
	
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
	protected:
		virtual ScaledPrice GetMarketOpenPrice() const;
		virtual ScaledPrice GetMarketClosePrice() const;
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
	protected:
		virtual ScaledPrice GetMarketOpenPrice() const;
		virtual ScaledPrice GetMarketClosePrice() const;
	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
