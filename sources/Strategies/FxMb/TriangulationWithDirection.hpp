/**************************************************************************
 *   Created: 2015/01/20 06:44:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Y.hpp"
#include "TriangulationWithDirectionTriangle.hpp"
#include "TriangulationWithDirectionTypes.hpp"
#include "TriangulationWithDirectionReport.hpp"
#include "Core/Strategy.hpp"

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {

	class TriangulationWithDirection : public Strategy {
		
	public:
		
		typedef Strategy Base;

	private:

		typedef std::vector<ScaledPrice> BookSide;

		struct Detection {

			Pair fistLeg;
			Y y;

			boost::array<PairSpeed, numberOfPairs> speed;

		};

		enum ProfitLossTest {
			PLT_LOSS = -1,
			PLT_NONE = 0,
			PLT_PROFIT_WAIT,
			PLT_PROFIT,
		};

	public:

		explicit TriangulationWithDirection(
				Context &,
				const std::string &,
				const Lib::IniSectionRef &);
		virtual ~TriangulationWithDirection();

	public:

		const YDirection & GetYDetectedDirection() const {
			return m_yDetected;
		}

	public:
		
		virtual void OnServiceStart(const Service &);
		virtual void OnServiceDataUpdate(
				const Service &,
				const Lib::TimeMeasurement::Milestones &);
		virtual void OnPositionUpdate(trdk::Position &);

	private:

		void UpdateBestBidAsk(const Service &);

		bool Detect(Detection &result) const;
		
		void CheckNewTriangle(const Lib::TimeMeasurement::Milestones &);
		void CheckTriangleCompletion(const Lib::TimeMeasurement::Milestones &);

		bool CheckProfitLoss(Twd::Position &firstLeg, bool isJustOpened);

		ProfitLossTest CheckLeg(const Twd::Position &) const;

		void OnCancel(const char *reason, const Twd::Position &reasonOrder);

	protected:

		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &);

	private:

		const size_t m_levelsCount;
		const Qty m_qty;
		const boost::posix_time::time_duration m_takeProfitWaitTime;

		ReportsState m_reports;

		BestBidAskPairs m_bestBidAsk;
	
		YDirection m_yDetected;
		YDirection m_yDetectedReported;
		const double m_yReportStep;

		Triangle::Id m_lastTriangleId;
		std::unique_ptr<Triangle> m_triangle;

	};

} } } }
