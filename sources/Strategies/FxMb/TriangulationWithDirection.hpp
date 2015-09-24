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
			boost::array<Leg, numberOfPairs> legs;
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

		size_t CalcBookUpdatesNumber() const;

	protected:
		
		virtual void OnServiceStart(const Service &);
		virtual void OnServiceDataUpdate(
				const Service &,
				const Lib::TimeMeasurement::Milestones &);
		virtual void OnPositionUpdate(trdk::Position &);
		virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);
		virtual void OnStopRequest(const trdk::StopMode &);
		virtual void OnPostionsCloseRequest();

	private:

		void UpdateDirection(const Service &);

		bool Detect(Detection &result) const;
		bool DetectByY1(Detection &result) const;
		bool DetectByY2(Detection &result) const;
		void CalcSpeed(const Y &, Detection &result) const;
		
		void CheckNewTriangle(const Lib::TimeMeasurement::Milestones &);
		bool CheckTriangleCompletion(const Lib::TimeMeasurement::Milestones &);

		bool StartScheduledLeg();

		bool CheckProfitLoss(Twd::Position &firstLeg, bool isJustOpened);

		ProfitLossTest CheckLeg(const Twd::Position &) const;

		void OnCancel(
				const char *reason,
				const Twd::Position &reasonOrder,
				const Lib::TimeMeasurement::PeriodFromStart &);

		bool CheckStopRequest(const trdk::StopMode &);
		bool CheckCurrentStopRequest();

		void OnLeg1Execution(trdk::Position &);
		void OnLeg2Execution(trdk::Position &);
		void OnLeg3Execution(trdk::Position &);

	private:

		boost::uuids::random_generator m_generateUuid;

		const size_t m_bookLevelsCount;
		const bool m_useAdjustedBookForTrades;
		const bool m_allowLeg1Closing;
		Qty m_qty;
		size_t m_trianglesLimit;
		size_t m_trianglesRest;

		ReportsState m_reports;

		BestBidAskPairs m_bestBidAsk;
	
		YDirection m_yDetected;
		boost::array<
				boost::array<size_t, numberOfPairs>,
				numberOfYs>
			m_detectedEcns;
		YDirection m_yDetectedReported;
		const double m_yReportStep;

		std::unique_ptr<Triangle> m_triangle;

		Leg m_scheduledLeg;

		std::unique_ptr<Triangle> m_prevTriangle;
		boost::posix_time::ptime m_prevTriangleTime;

		struct Stat {
			boost::accumulators::accumulator_set<
					double,
					boost::accumulators::stats<
						boost::accumulators::tag::count,
						boost::accumulators::tag::mean>>
				winners;
			boost::accumulators::accumulator_set<
					double,
					boost::accumulators::stats<
						boost::accumulators::tag::count,
						boost::accumulators::tag::mean>>
				losers;
		} m_stat;

	};

} } } }
