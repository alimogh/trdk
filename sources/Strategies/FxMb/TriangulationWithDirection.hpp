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
			PairsSpeed speed;
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

		void UpdateDirection(const Service &);

		bool Detect(Detection &result) const;
		bool DetectByY1(Detection &result) const;
		bool DetectByY2(Detection &result) const;
		void CalcSpeed(const Y &, Detection &result) const;
		
		void CheckNewTriangle(const Lib::TimeMeasurement::Milestones &);
		bool CheckTriangleCompletion(const Lib::TimeMeasurement::Milestones &);

		void StartScheduledLeg();

		bool CheckProfitLoss(Twd::Position &firstLeg, bool isJustOpened);

		ProfitLossTest CheckLeg(const Twd::Position &) const;

		void OnCancel(const char *reason, const Twd::Position &reasonOrder);

	protected:

		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &);

	private:

		const size_t m_levelsCount;
		const bool m_allowLeg1Closing;

		const Qty m_qtyA;
		const Qty m_qtyB;

		ReportsState m_reports;

		BestBidAskPairs m_bestBidAsk;
	
		YDirection m_yDetected;
		boost::array<
				boost::array<size_t, numberOfPairs>,
				numberOfYs>
			m_detectedEcns;
		YDirection m_yDetectedReported;
		const double m_yReportStep;

		Triangle::Id m_lastTriangleId;
		std::unique_ptr<Triangle> m_triangle;

		Leg m_scheduledLeg;

	};

} } } }
