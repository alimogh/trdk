/**************************************************************************
 *   Created: 2012/07/10 01:25:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Algo.hpp"

namespace Strategies { namespace QuickArbitrage {

	class Algo : public ::Algo {

	public:

		typedef ::Algo Base;

		enum PositionState {
			STATE_OPENING				= 1,
			STATE_CLOSING_TRY_STOP_LOSS	= 2,
			STATE_CLOSING				= 3
		};

	public:

		explicit Algo(boost::shared_ptr<DynamicSecurity>, const char *logTag);
		virtual ~Algo();

	public:

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);
		virtual void ClosePositionsAsIs(PositionBandle &);
		
		virtual void ReportDecision(const Position &) const;

		virtual bool IsLongPosEnabled() const = 0;
		virtual bool IsShortPosEnabled() const = 0;
		virtual Security::Price GetLongPriceMod() const = 0;
		virtual Security::Price GetShortPriceMod() const = 0;
		virtual Security::Price GetTakeProfit() const = 0;
		virtual Security::Price GetStopLoss() const = 0;
		virtual Security::Price GetVolume() const = 0;
 
	protected:

		void ReportStopLossTry(const Position &) const;
		void ReportStopLossDo(const Position &) const;

		virtual void ClosePosition(Position &, bool asIs) = 0;

		void CloseLongPositionStopLossDo(Position &);
		void CloseShortPositionStopLossDo(Position &);

		void CloseLongPositionStopLossTry(Position &);
		void CloseShortPositionStopLossTry(Position &);

	private:

		boost::shared_ptr<Position> OpenLongPosition();
		boost::shared_ptr<Position> OpenShortPosition();

		void ClosePositionStopLossTry(Position &);

	protected:

		const char *const m_logTag;

	};

} }
