/**************************************************************************
 *   Created: 2014/11/30 05:05:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirection.hpp"
#include "TriangulationWithDirectionPosition.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/TradingLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {
	class TriangulationWithDirection;
} } } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

////////////////////////////////////////////////////////////////////////////////

TriangulationWithDirection::TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirection", tag),
	m_levelsCount(
		conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
	m_qty(conf.ReadTypedKey<Qty>("qty")),
	m_takeProfitWaitTime(
		pt::seconds(conf.ReadTypedKey<long>("take_profit_wait_time_sec"))),
	m_reports(
		GetContext(),
		conf.ReadTypedKey<double>("commission"),
		conf.ReadBoolKey("log.strategy"),
		conf.ReadBoolKey("log.pnl")),
	m_yReportStep(conf.ReadTypedKey<double>("log.y_report_step")),
	m_lastTriangleId(0) {

	{
		const BestBidAsk def = {};
		m_bestBidAsk.fill(def);
	}
	m_yDetected.fill(.0);
	m_yDetectedReported.fill(.0);

}

TriangulationWithDirection::~TriangulationWithDirection() {
	//...//
}

void TriangulationWithDirection::OnServiceStart(const Service &service) {

	const BestBidAsk statService = {
		boost::polymorphic_downcast<const StatService *>(&service)
	};
	const auto &symbol
		= statService.service->GetSecurity(0).GetSymbol().GetSymbol();
	const size_t index
		= symbol == "EUR/USD"
			?	PAIR_AB
			:	symbol == "EUR/JPY"
				?	PAIR_AC
				:	PAIR_BC;
	Assert(!m_bestBidAsk[index].service);
	m_bestBidAsk[index] = statService;

	{
		bool isFull = true;
		foreach (const auto &s, m_bestBidAsk) {
			if (!s.service) {
				isFull = false;
				break;
			}
		}
		if (isFull) {
			m_reports.WriteStrategyLogHead(GetContext(), m_bestBidAsk);
		}
	}

// @todo see https://trello.com/c/ONnb5ai2
//		m_stat.push_back(statService);

}

void TriangulationWithDirection::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &timeMeasurement) {

	UpdateBestBidAsk(service);

	if (m_triangle) {
		CheckTriangleCompletion(timeMeasurement);
	} else {
		CheckNewTriangle(timeMeasurement);
	}

}

void TriangulationWithDirection::OnPositionUpdate(trdk::Position &position) {
	
	Assert(m_triangle);

	if (position.IsError()) {
		Assert(IsBlocked());
		return;
	}

	if (position.HasActiveOrders()) {
		return;
	}

	Twd::Position &order = dynamic_cast<Twd::Position &>(position);

	if (!order.IsActive()) {
		//! @todo see https://trello.com/c/QOBSd8RZ
		return;
	}
	order.Deactivate();

	if (order.GetOpenedQty() == 0) {
		
		Assert(IsZero(order.GetCloseStartPrice()));

		switch (order.GetLeg()) {
			case LEG1:
				OnCancel("exec report", order);
				break;
			case LEG2:
				{
					Twd::Position &firstLeg = m_triangle->GetLeg(LEG1);
					Assert(IsZero(firstLeg.GetCloseStartPrice()));
					Assert(firstLeg.IsOpened());
					AssertLt(0, firstLeg.GetActiveQty());
					m_triangle->OnLeg2Cancel();
					if (!CheckProfitLoss(firstLeg, false)) {
						m_triangle->StartLeg2();
					}
				}
				break;
			case LEG3:
				m_triangle->OnLeg3Cancel();
				m_triangle->StartLeg3(TimeMeasurement::Milestones());
				break;
			default:
				AssertEq(LEG1, order.GetLeg());
				AssertFail("Unknown leg no.");
				break;
		}

		return;
		
	} else if (!IsZero(order.GetCloseStartPrice())) {
			
		AssertLt(0, order.GetOpenedQty());
		AssertEq(LEG1, order.GetLeg());
			
		if (order.GetActiveQty() == 0) {
			OnCancel("exec report", order);
			return;
		} else if (
				order.GetCloseType() != Position::CLOSE_TYPE_TAKE_PROFIT) {
			m_triangle->Cancel(order.GetCloseType());
			return;
		}

		order.SetCloseStartPrice(0);
		
	}
 
 	Assert(order.IsOpened());
	Assert(IsZero(order.GetCloseStartPrice()));
	AssertEq(0, order.GetClosedQty());
	Assert(!order.HasActiveOrders());

	switch (order.GetLeg()) {
			
		case LEG1:
			if (CheckProfitLoss(order, true)) {
				return;
			}
			m_triangle->StartLeg2();
			m_triangle->GetReport().ReportAction(
				"executed",
				"exec report",
				"1 -> 2",
				&order);
			break;

		case LEG3:
			m_triangle->GetReport().ReportAction(
				"executed",
				"exec report",
				order.GetLeg(),
				&order);
			m_triangle.reset();
			break;

		default:
			AssertEq(LEG2, order.GetLeg());
			m_triangle->GetReport().ReportAction(
				"executed",
				"exec report",
				order.GetLeg(),
				&order);
			break;
			
	}

}

void TriangulationWithDirection::UpdateBestBidAsk(const Service &service) {
		
	auto bestBidAskIt = std::find_if(
		m_bestBidAsk.begin(),
		m_bestBidAsk.end(),
		[&service](const BestBidAsk &bestBidAsk) {
			return bestBidAsk.service == &service;
		}); 
	Assert(bestBidAskIt != m_bestBidAsk.end());

	bestBidAskIt->Reset();
	const auto &ecnsCount = GetContext().GetMarketDataSourcesCount();
	bool hasNotOpportunity = false;
	for (size_t ecn = 0; !hasNotOpportunity && ecn < ecnsCount; ++ecn) {
		const Security &security = bestBidAskIt->service->GetSecurity(ecn);
		{
			const auto &bid = security.GetBidPrice();
			if (IsZero(bid)) {
				hasNotOpportunity = true;
			} else if (bestBidAskIt->bestBid.price < bid) {
				bestBidAskIt->bestBid.price = bid;
				bestBidAskIt->bestBid.source = ecn;
			}
		}
		{
			const auto &ask = security.GetAskPrice();
			if (IsZero(ask)) {
				hasNotOpportunity = true;
			} else if (
					bestBidAskIt->bestAsk.price > ask
					|| IsZero(bestBidAskIt->bestAsk.price)) {
				bestBidAskIt->bestAsk.price = ask;
				bestBidAskIt->bestAsk.source = ecn;
			}
		}
	}

	if (hasNotOpportunity) {
			
		bestBidAskIt->Reset();
			
		ResetYDirection(m_yDetected);
		if (m_triangle) {
			m_triangle->ResetYDirection();
		}
		
	} else if (
			IsZero(m_bestBidAsk[PAIR_AB].bestBid.price)
			||	IsZero(m_bestBidAsk[PAIR_AB].bestAsk.price)
			||	IsZero(m_bestBidAsk[PAIR_BC].bestBid.price)
			||	IsZero(m_bestBidAsk[PAIR_BC].bestAsk.price)
			||	IsZero(m_bestBidAsk[PAIR_AC].bestBid.price)
			||	IsZero(m_bestBidAsk[PAIR_AC].bestAsk.price)) {
		
		ResetYDirection(m_yDetected);
		if (m_triangle) {
			m_triangle->ResetYDirection();
		}

	} else {
			
		CalcYDirection(
			m_bestBidAsk[PAIR_AB].bestBid.price,
			m_bestBidAsk[PAIR_AB].bestAsk.price,
			m_bestBidAsk[PAIR_BC].bestBid.price,
			m_bestBidAsk[PAIR_BC].bestAsk.price,
			m_bestBidAsk[PAIR_AC].bestBid.price,
			m_bestBidAsk[PAIR_AC].bestAsk.price,
			m_yDetected);

		if (m_triangle) {
			m_triangle->UpdateYDirection();
		}

	}

	const auto &isTimeToReport = [this](double current, double reported) {
		return
			reported > 1.0 != current > 1.0
			|| current < (reported - m_yReportStep)
			|| (reported + m_yReportStep) < current;
	};
	if (
			isTimeToReport(m_yDetected[Y1], m_yDetectedReported[Y1])
			|| isTimeToReport(m_yDetected[Y2], m_yDetectedReported[Y2])) {
		GetTradingLog().Write(
			"\topportunity\tY1: %1% -> %2%\tY2: %3% -> %4%",
			[this](TradingRecord &record) {
				record
					% m_yDetectedReported[Y1]
					% m_yDetected[Y1]
					% m_yDetectedReported[Y2]
					% m_yDetected[Y2];
			});
		m_yDetectedReported = m_yDetected;
	}

}

bool TriangulationWithDirection::Detect(Detection &result) const {

	AssertNe(Y_UNKNOWN, CheckOpportunity(m_yDetected));

	for (size_t pair = 0; pair < result.speed.size(); ++pair) {
			
		PairSpeed &speed = result.speed[pair];
		const auto &bestBidAsk = m_bestBidAsk[pair];

		{
			const auto &data
				= bestBidAsk.service->GetData(bestBidAsk.bestBid.source);
			const bool isRising
				= data.current.theo > data.current.emaFast
					&& data.current.emaFast > data.current.emaSlow;
			speed.rising = isRising
				?	(1.0 / data.prev2.theo) * data.current.theo
				:	std::numeric_limits<double>::quiet_NaN();
		}

		{
			const auto &data
				= bestBidAsk.service->GetData(bestBidAsk.bestAsk.source);
			const bool isFalling
				= data.current.theo < data.current.emaFast
					&& data.current.emaFast < data.current.emaSlow;
			speed.falling = isFalling
				?	data.prev2.theo * (1.0 / data.current.theo)
				:	std::numeric_limits<double>::quiet_NaN();
		}

	}

	if (m_yDetected[Y1] >= 1.0) {
		if (
				!isnan(result.speed[PAIR_AB].falling)
				&& result.speed[PAIR_AB].falling > 0
				&& !isnan(result.speed[PAIR_BC].rising)
				&& result.speed[PAIR_BC].rising > 0
				&& isnan(result.speed[PAIR_AC].rising)
				&& isnan(result.speed[PAIR_AC].falling)) {
			result.y = Y1;
			result.fistLeg = PAIR_AB;
			return true;
		} else if (
				!isnan(result.speed[PAIR_AC].rising)
				&& result.speed[PAIR_AC].rising > 0
				&& !isnan(result.speed[PAIR_BC].falling)
				&& result.speed[PAIR_BC].falling > 0
				&& isnan(result.speed[PAIR_AB].rising)
				&& isnan(result.speed[PAIR_AB].falling)) {
			result.y = Y1;
			result.fistLeg = PAIR_AC;
			return true;
		}
	}

	if (m_yDetected[Y2] >= 1.0) {
		if (
				!isnan(result.speed[PAIR_AB].rising)
				&& result.speed[PAIR_AB].rising > 0
				&& !isnan(result.speed[PAIR_BC].falling)
				&& result.speed[PAIR_BC].falling > 0
				&& isnan(result.speed[PAIR_AC].rising)
				&& isnan(result.speed[PAIR_AC].falling)) {
			result.y = Y2;
			result.fistLeg = PAIR_AB;
			return true;
		} else if (
				!isnan(result.speed[PAIR_AC].falling)
				&& result.speed[PAIR_AC].falling > 0
				&& !isnan(result.speed[PAIR_BC].rising)
				&& result.speed[PAIR_BC].rising > 0
				&& isnan(result.speed[PAIR_AB].rising)
				&& isnan(result.speed[PAIR_AB].falling)) {
			result.y = Y2;
			result.fistLeg = PAIR_AC;
			return true;
		}
	}

	return false;

}

void TriangulationWithDirection::CheckNewTriangle(
		const TimeMeasurement::Milestones &timeMeasurement) {

	Assert(!m_triangle);

 	if (CheckOpportunity(m_yDetected) == Y_UNKNOWN) {
		timeMeasurement.Measure(
			TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
 		return;
 	}

	timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START);

	Detection detection;
	if (!Detect(detection)) {
		return;
	}
	std::unique_ptr<Triangle> triangle(
		new Triangle(
			*this,
			m_reports,
			++m_lastTriangleId,
			detection.y,
			m_qty,
			{
				PAIR_AB,
				detection.fistLeg == PAIR_AB ? LEG1 : LEG2,
				detection.y == Y2,
				detection.fistLeg == PAIR_AB,
			},
			{
				PAIR_BC,
				LEG3,
				detection.y == Y2,
				detection.fistLeg == PAIR_AB,
			},
			{
				PAIR_AC,
				detection.fistLeg == PAIR_AC ? LEG1 : LEG2,
				detection.y == Y1,
				detection.fistLeg == PAIR_AC,
			},
			m_bestBidAsk));

	try {
		triangle->StartLeg1(timeMeasurement, detection.speed);
	} catch (const HasNotMuchOpportunityException &ex) {
		GetLog().Warn("Failed to start triangle: \"%1%\"", ex);
	}

	Assert(!m_triangle);
	triangle.swap(m_triangle);

}

void TriangulationWithDirection::CheckTriangleCompletion(
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	Assert(m_triangle);

	if (
			!m_triangle->IsLegExecuted(LEG1)
			||	!m_triangle->IsLegExecuted(LEG2)
			||	m_triangle->IsLegStarted(LEG3)) {
		return;
	}

	Triangle::PairInfo &leg3Info = m_triangle->GetPair(LEG3);
	Security &security = leg3Info.GetBestSecurity();
	const auto &ecn = security.GetSource().GetIndex();
	const auto &data = leg3Info.bestBidAsk->service->GetData(ecn);
	if (leg3Info.isBuy) {
		if (data.current.theo > data.current.emaSlow) {
			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
			return;
		}
	} else {
		if (data.current.theo < data.current.emaSlow) {
			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
			return;
		}
	}

	timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START);
	m_triangle->StartLeg3(security, timeMeasurement);

}

bool TriangulationWithDirection::CheckProfitLoss(
		Twd::Position &firstLeg,
		bool isJustOpened) {
	Position::CloseType closeType;
	const char *closeTypeStr;
	switch (CheckLeg(firstLeg)) {
		case PLT_LOSS:
			closeType = Position::CLOSE_TYPE_STOP_LOSS;
			closeTypeStr = "loss detected";
			break;
		case PLT_PROFIT:
			closeType = Position::CLOSE_TYPE_TAKE_PROFIT;
			closeTypeStr = "profit detected";
			break;
		default:
			return false;
	}
	if (isJustOpened) {
		m_triangle->GetReport().ReportAction(
			"executed",
			"exec report",
			firstLeg.GetLeg(),
			&firstLeg);
	}
	m_triangle->Cancel(closeType);
	m_triangle->GetReport().ReportAction(
		"canceling",
		closeTypeStr,
		firstLeg.GetLeg());
	return true;
}

TriangulationWithDirection::ProfitLossTest TriangulationWithDirection::CheckLeg(
		const Twd::Position &leg)
		const {

	Assert(!leg.HasActiveOrders());

	const bool isLong = leg.GetType() == Position::TYPE_LONG;
	const Security &security = leg.GetSecurity();

	const auto &printTradingRecordStart = [&](TradingRecord &record) {
		record
			% security
			% security.GetSource().GetTag()
			% m_triangle->GetId()
			% (isLong ? "long" : "short");
	};

	if (!m_triangle->HasOpportunity()) {
		GetTradingLog().Write(
			"\tloss-detected\t%1%\t%2%\topp.: %3%\t%4%"
				"\tY1 = %5%, Y2 = %6%, current: Y%7%",
			[&](TradingRecord &record) {
				printTradingRecordStart(record);
				record
					% m_triangle->GetYDirection()[Y1] 
					% m_triangle->GetYDirection()[Y2]
					% GetYNo(m_triangle->GetY());
			});
		return PLT_LOSS;
	}

	const auto &openPrice = security.DescalePrice(leg.GetOpenPrice());
	double seenProfit;
	double currentPrice;
	if (isLong) {
		currentPrice = security.GetBidPrice();
		seenProfit = currentPrice - openPrice;
	} else {
		currentPrice = security.GetAskPrice();
		seenProfit = openPrice - currentPrice;
	}
	if (seenProfit > 0) {
		const auto &now = GetContext().GetCurrentTime();
		if (m_triangle->GetTakeProfitTime() == pt::not_a_date_time) {
			m_triangle->SetTakeProfitTime(now + m_takeProfitWaitTime);
			GetTradingLog().Write(
				"\tprofit-detected\t%1%\t%2%\topp.: %3%\t%4%"
					"\t%5% -> %6%  = %7$.7f at %8%",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record
						% openPrice
						% currentPrice
						% seenProfit
						% m_triangle->GetTakeProfitTime();
				});
		} else if (m_triangle->GetTakeProfitTime() <= now) {
			GetTradingLog().Write(
				"\tprofit\t%1%\t%2%\topp.: %3%\t%4%\t%5% -> %6%  = %7$.7f",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record % openPrice % currentPrice % seenProfit;
				});
			return PLT_PROFIT;
		}	
	} else if (m_triangle->GetTakeProfitTime() != pt::not_a_date_time) {
		GetTradingLog().Write(
			"\tprofit-cancel\t%1%\t%2%\topp.: %3%\t%4% at %5%",
			[&](TradingRecord &record) {
				printTradingRecordStart(record);
				record % m_triangle->GetTakeProfitTime();
			});
		m_triangle->ResetTakeProfitTime();
	}

	return PLT_NONE;

}

void TriangulationWithDirection::OnCancel(
		const char *reason,
		const Twd::Position &reasonOrder) {
	Assert(m_triangle);
	m_triangle->GetReport().ReportAction(
		"canceled",
		reason,
		reasonOrder.GetLeg(),
		&reasonOrder);
	m_triangle.reset();
	CheckNewTriangle(TimeMeasurement::Milestones());
}

void TriangulationWithDirection::UpdateAlogImplSettings(
		const IniSectionRef &) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Strategy> CreateTriangulationWithDirectionStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return boost::shared_ptr<Strategy>(
		new TriangulationWithDirection(context, tag, conf));
}

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Strategy> CreateTwdStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	return CreateTriangulationWithDirectionStrategy(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
