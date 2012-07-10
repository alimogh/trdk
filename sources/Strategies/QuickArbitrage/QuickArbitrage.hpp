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

	public:

		explicit Algo(boost::shared_ptr<DynamicSecurity>);
		virtual ~Algo();

	public:

		virtual const std::string & GetName() const;

	public:

		virtual void Update();

		virtual boost::shared_ptr<PositionBandle> OpenPositions();
		virtual void ClosePositions(PositionBandle &);
		
		virtual void ReportDecision(const Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		void ReportStopLoss(const Position &) const;

	private:

		boost::shared_ptr<Position> OpenLongPosition();
		boost::shared_ptr<Position> OpenShortPosition();

		void ClosePosition(Position &);
		void CloseLongPosition(Position &);
		void CloseShortPosition(Position &);

	};

} }
