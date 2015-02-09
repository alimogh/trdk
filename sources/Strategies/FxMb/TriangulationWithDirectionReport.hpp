
/**************************************************************************
 *   Created: 2015/01/18 11:04:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TriangulationWithDirectionTypes.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	////////////////////////////////////////////////////////////////////////////////

	struct ReportsState : private boost::noncopyable {
		
		class Startegy;
		Startegy *strategy;

		class Pnl;
		Pnl *pnl;

		bool enablePriceUpdates;

		ReportsState(
				Context &,
				double commission,
				bool enableStrategyLog,
				bool enablePriceUpdates,
				bool enablePnlLog);
		~ReportsState();

		void WriteStrategyLogHead(const Context &, const BestBidAskPairs &);

	};

	////////////////////////////////////////////////////////////////////////////////

	class TriangleReport : private boost::noncopyable {

	public:

		explicit TriangleReport(const Triangle &triangle, ReportsState &state)
			: m_triangle(triangle),
			m_state(state) {
		}

	public:

		void ReportAction(
				const char *action,
				const char *reason,
				const Leg &actionLeg,
				const Lib::TimeMeasurement::PeriodFromStart &delay,
				const Twd::Position *const reasonOrder = nullptr);
		void ReportAction(
				const char *action,
				const char *reason,
				const char *actionLegs,
				const Lib::TimeMeasurement::PeriodFromStart &delay,
				const Twd::Position *const reasonOrder = nullptr,
				const PairsSpeed *speed = nullptr);

		void ReportUpdate();

	private:

		const Triangle &m_triangle;
		ReportsState &m_state;

	};

	////////////////////////////////////////////////////////////////////////////////

} } } }
