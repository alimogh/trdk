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

		Security::Price CalcLongPrice() const;
		Security::Price CalcLongTakeProfit(Security::Price openPrice) const;
		Security::Price CalcLongStopLoss(Security::Price openPrice) const;
		
		Security::Price CalcShortPrice() const;
		Security::Price CalcShortTakeProfit(Security::Price openPrice) const;
		Security::Price CalcShortStopLoss(Security::Price openPrice) const;

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

	};

} }
