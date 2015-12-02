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
#include "Core/RiskControl.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

////////////////////////////////////////////////////////////////////////////////

namespace {

	const size_t nTrianglesLimit = std::numeric_limits<size_t>::max();

}

namespace {

	bool IsProfit(
			const Triangle::PairInfo &pair,
			const StatService::Stat &data) {
		return
			//! @sa TRDK-241: VWAP crosses slowEMA, this is your signal to buy
			//! leg 3 if it falls or sell leg 3 if it rises:
			(pair.isBuy
				?	data.history.back().vwapAsk > data.history.back().emaSlow
				:	data.history.back().vwapBid < data.history.back().emaSlow)
			&& pair.GetCurrentPrice() > 0;
	}

	size_t ReadMaxTrianglesCount(const IniSectionRef &conf, Module::Log &log) {
		const char *const keyName = "risk_control.triangles_limit";
		if (
				boost::iequals(
					conf.ReadTypedKey<std::string>(keyName),
					"unlimited")) {
			log.Info("Maximum triangles count: UNLIMITED.");
			return nTrianglesLimit;
		} else {
			const auto result = conf.ReadTypedKey<size_t>(keyName);
			log.Info("Maximum triangles count: %1% triangles.", result);
			return result;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////

TriangulationWithDirection::TriangulationWithDirection(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirection", tag, conf)
	, m_allowLeg1Closing(
		conf.ReadTypedKey<bool>("allow_leg1_closing"))
	, m_qty(conf.ReadTypedKey<Qty>("invest_amount"))
	, m_trianglesLimit(ReadMaxTrianglesCount(conf, GetLog()))
	, m_trianglesRest(m_trianglesLimit)
	, m_reports(
		GetContext(),
		GetId(),
		conf.ReadBoolKey("log.strategy"),
		conf.ReadBoolKey("log.updates"))
	, m_yReportStep(conf.ReadTypedKey<double>("log.y_report_step"))
	, m_scheduledLeg(LEG_UNKNOWN) {

	m_pairsOrder[PAIR_AB] = conf.ReadKey("pair_1");
	m_pairsOrder[PAIR_BC] = conf.ReadKey("pair_2");
	m_pairsOrder[PAIR_AC] = conf.ReadKey("pair_3");

	{
		const BestBidAsk def = {};
		m_bestBidAsk.fill(def);
	}
	m_yDetected.fill(.0);
	m_yDetectedReported.fill(.0);

#	ifdef DEV_VER
		m_detectedEcns[Y1].fill(std::numeric_limits<size_t>::max());
		m_detectedEcns[Y2].fill(std::numeric_limits<size_t>::max());
#	endif

	GetLog().Info(
		"Allow Leg 1 closing: %1%.",
		m_allowLeg1Closing ? "yes" : "no");

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
	auto orderIt = std::find(
		m_pairsOrder.begin(),
		m_pairsOrder.end(),
		symbol);
	if (orderIt == m_pairsOrder.end()) {
		GetLog().Error("Failed to set %1% - pair order is unknown.", symbol);
		throw ModuleError("Pair order is unknown");
	}
	orderIt->clear();
	
	const size_t index = std::distance(m_pairsOrder.begin(), orderIt);
	GetLog().Info("Using %1% as pair #%2%",  symbol, index + 1);
	AssertLt(index, numberOfPairs);
	Assert(!m_bestBidAsk[index].service);
	m_bestBidAsk[index] = statService;

	bool isFull = true;
	foreach (const auto &s, m_bestBidAsk) {
		if (!s.service) {
			isFull = false;
			break;
		}
	}
	if (isFull) {
		foreach (const auto &pair, m_pairsOrder) {
			if (!pair.empty()) {
				GetLog().Error("Pair %1% ordered by was not set.", pair);
				throw ModuleError("Pair was not ordered");
			}
		}
		m_reports.WriteStrategyLogHead(GetContext(), m_bestBidAsk);
	}

}

void TriangulationWithDirection::OnServiceDataUpdate(
		const Service &service,
		const TimeMeasurement::Milestones &timeMeasurement) {

	UpdateDirection(service);

	if (m_triangle) {
	
		if (!StartScheduledLeg()) {
			return;
		}

		if (!CheckTriangleCompletion(timeMeasurement)) {
			m_triangle->GetReport().ReportUpdate();
		}
	
	} else if (GetPositions().IsEmpty()) {

		AssertEq(LEG_UNKNOWN, m_scheduledLeg);
		Assert(
			!GetContext().GetDropCopy()
			|| GetRiskControlScope().GetStatistics().empty());

		if (m_prevTriangle && m_prevTriangle->IsLegExecuted(LEG3)) {
			const auto timeFromClose
				= GetContext().GetCurrentTime() - m_prevTriangleTime;
			if (timeFromClose <= pt::seconds(30)) {
				m_prevTriangle->GetReport().ReportUpdate();
			}
		}

		CheckNewTriangle(timeMeasurement);

	}

}

bool TriangulationWithDirection::StartScheduledLeg() {

	Assert(m_triangle);

	try {
		static_assert(numberOfLegs == 3, "Legs list changed.");
		switch (m_scheduledLeg) {
			case LEG2:
				m_triangle->StartLeg2();
				break;
			case LEG3:
				m_triangle->StartLeg3(TimeMeasurement::Milestones(), true);
				break;
			default:
				AssertEq(LEG_UNKNOWN, m_scheduledLeg);
				break;
			case LEG_UNKNOWN:
				return true;
		}
	} catch (const HasNotMuchOpportunityException &ex) {
		GetLog().Warn(
			"Failed to start scheduled leg %1%: \"%2%\". Required: %3% %4%.",
			GetLegNo(m_scheduledLeg),
			ex,
			ex.GetRequiredQty(),
			ex.GetSecurity());
		return false;
	} catch (const PriceNotChangedException &) {
		GetTradingLog().Write(
			"\twaiting for changes to execute leg %1%",
			[this](TradingRecord &record) {
				record % GetLegNo(m_scheduledLeg);
			});
		return false;
	}

	m_scheduledLeg = LEG_UNKNOWN;
	return true;

}

void TriangulationWithDirection::OnPositionUpdate(trdk::Position &position) {

	AssertEq(LEG_UNKNOWN, m_scheduledLeg);

	if (!m_triangle) {
		// Closing opened position.
		if (!position.IsCompleted()) {
			GetLog().Error(
				"Failed to close %1% position.",
				position.GetSecurity());
			Block();
		} else {
			GetLog().Info("%1% position closed.", position.GetSecurity());
		}
		return;
	}

	AssertLt(0, m_trianglesRest);
	
	if (position.IsError()) {
		Assert(IsBlocked());
		return;
	}

	if (position.HasActiveOrders()) {
		return;
	}

	Assert(!position.IsCompleted() || position.GetOpenedQty() == 0);

	Twd::Position &order = dynamic_cast<Twd::Position &>(position);

	if (order.GetOpenedQty() == 0) {
	
		Assert(order.IsCompleted());
		Assert(IsZero(order.GetCloseStartPrice()));

		switch (order.GetLeg()) {
			case LEG1:
				{
					const auto execReportDelay
						= position.GetTimeMeasurement().Measure(
							TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY_1);
					OnCancel("exec report", order, execReportDelay);
				}
				CheckCurrentStopRequest();
				break;
			case LEG2:
				{
					Twd::Position &firstLeg = m_triangle->GetLeg(LEG1);
					Assert(IsZero(firstLeg.GetCloseStartPrice()));
					Assert(firstLeg.IsOpened());
					AssertLt(0, firstLeg.GetActiveQty());
					m_triangle->OnLeg2Cancel();
					if (CheckCurrentStopRequest()) {
						break;
					} else if (!CheckProfitLoss(firstLeg, false)) {
						try {
							m_triangle->StartLeg2();
						} catch (const HasNotMuchOpportunityException &ex) {
							m_scheduledLeg = LEG2;
							GetLog().Warn(
								"Failed to start leg 2: \"%1%\"."
									" Required: %2% %3%."
									" Scheduled.",
								ex,
								ex.GetRequiredQty(),
								ex.GetSecurity());
							return;
						} catch (const PriceNotChangedException &) {
							m_scheduledLeg = LEG2;
							return;
						}
					}
				}
				break;
			case LEG3:
				m_triangle->OnLeg3Cancel();
				if (CheckCurrentStopRequest()) {
					break;
				}
				try {
					m_triangle->StartLeg3(position.GetTimeMeasurement(), true);
				} catch (const HasNotMuchOpportunityException &ex) {
					m_scheduledLeg = LEG3;
					GetLog().Warn(
						"Failed to start leg 3: \"%1%\"."
							" Required: %2% %3%."
							" Scheduled.",
						ex,
						ex.GetRequiredQty(),
						ex.GetSecurity());
					return;
				} catch (const PriceNotChangedException &) {
					m_scheduledLeg = LEG3;
					return;
				}
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
			Assert(order.IsCompleted());
			const auto &execReportDelay = position.GetTimeMeasurement().Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY_1);
			OnCancel("exec report", order, execReportDelay);
			CheckCurrentStopRequest();
			return;
		} else if (
				order.GetCloseType() != Position::CLOSE_TYPE_TAKE_PROFIT) {
			Assert(!order.IsCompleted());
			position.GetTimeMeasurement().Measure(
				TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY_1);
			if (CheckCurrentStopRequest()) {
				return;
			}
			m_triangle->Cancel(order.GetCloseType());
			return;
		}

		order.SetCloseStartPrice(0);

	}

	Assert(!order.IsCompleted()); 
 	Assert(order.IsOpened());
	Assert(IsZero(order.GetCloseStartPrice()));
	AssertEq(0, order.GetClosedQty());
	Assert(!order.HasActiveOrders());

	const auto &leg = order.GetLeg();
	switch (leg) {
		case LEG1:
			OnLeg1Execution(position);
			break;
		case LEG2:
			OnLeg2Execution(position);
			break;
		case LEG3:
			OnLeg3Execution(position);
			break;
		default:
			AssertEq(LEG1, leg);
			throw LogicError("Leg is unknown");
	}

}

void TriangulationWithDirection::OnStopRequest(const StopMode &stopMode) {
	CheckStopRequest(stopMode);
}

bool TriangulationWithDirection::CheckCurrentStopRequest() {
	return CheckStopRequest(GetStopMode());
}

bool TriangulationWithDirection::CheckStopRequest(const StopMode &stopMode) {
	static_assert(numberOfStopModes == 3, "Stop mode list changed.");
	switch (stopMode) {
		default:
			AssertEq(STOP_MODE_IMMEDIATELY, stopMode);
		case STOP_MODE_IMMEDIATELY:
			ReportStop();
			return true;
		case STOP_MODE_GRACEFULLY_ORDERS:
			if (!m_triangle || !m_triangle->HasActiveOrders()) {
				ReportStop();
			}
			return true;
		case STOP_MODE_GRACEFULLY_POSITIONS:
			if (!m_triangle) {
				ReportStop();
			}
			return true;
		case STOP_MODE_UNKNOWN:
			return false;
	}
}

void TriangulationWithDirection::CalcYDirection() {

	Twd::CalcYDirection(
		m_bestBidAsk[PAIR_AB].bestBid.price,
		m_bestBidAsk[PAIR_AB].bestAsk.price,
		m_bestBidAsk[PAIR_BC].bestBid.price,
		m_bestBidAsk[PAIR_BC].bestAsk.price,
		m_bestBidAsk[PAIR_AC].bestBid.price,
		m_bestBidAsk[PAIR_AC].bestAsk.price,
		m_yDetected);

	//! @sa TRDK-209:
#	ifdef DEV_VER
		const auto maxAllowedY = 1.10095;
#	else
		const auto maxAllowedY = 1.00095;
#	endif
	if (m_yDetected[Y1] > maxAllowedY || m_yDetected[Y2] > maxAllowedY) {
		boost::format message(
			"Wrong detection detected: %1%/%2%"
				" by prices %3%/%4%, %5%/%6%, %7%/%8%");
		message
			% m_yDetected[Y1]
			% m_yDetected[Y2]
			% m_bestBidAsk[PAIR_AB].bestBid.price
			% m_bestBidAsk[PAIR_AB].bestAsk.price
			% m_bestBidAsk[PAIR_BC].bestBid.price
			% m_bestBidAsk[PAIR_BC].bestAsk.price
			% m_bestBidAsk[PAIR_AC].bestBid.price
			% m_bestBidAsk[PAIR_AC].bestAsk.price;
		throw RiskControlException(message.str().c_str());
	}

}

void TriangulationWithDirection::UpdateDirection(const Service &service) {
		
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
		
#		ifdef DEV_VER
			m_detectedEcns[Y1].fill(std::numeric_limits<size_t>::max());
			m_detectedEcns[Y2].fill(std::numeric_limits<size_t>::max());
#		endif

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

#		ifdef DEV_VER
			m_detectedEcns[Y1].fill(std::numeric_limits<size_t>::max());
			m_detectedEcns[Y2].fill(std::numeric_limits<size_t>::max());
#		endif

	} else {
			
		CalcYDirection();

		if (CheckOpportunity(m_yDetected) != Y_UNKNOWN) {
			m_detectedEcns[Y1][PAIR_AB] = m_bestBidAsk[PAIR_AB].bestBid.source;
			m_detectedEcns[Y1][PAIR_BC] = m_bestBidAsk[PAIR_BC].bestBid.source;
			m_detectedEcns[Y1][PAIR_AC] = m_bestBidAsk[PAIR_AC].bestAsk.source;
			m_detectedEcns[Y2][PAIR_AB] = m_bestBidAsk[PAIR_AB].bestAsk.source;
			m_detectedEcns[Y2][PAIR_BC] = m_bestBidAsk[PAIR_BC].bestAsk.source;
			m_detectedEcns[Y2][PAIR_AC] = m_bestBidAsk[PAIR_AC].bestBid.source;
		} else {
#			ifdef DEV_VER
				m_detectedEcns[Y1].fill(std::numeric_limits<size_t>::max());
				m_detectedEcns[Y2].fill(std::numeric_limits<size_t>::max());
#			endif
		}
		
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

namespace {

	class SpeedStat : private boost::noncopyable {

	public:

		typedef StatService::Point Point;
		typedef StatService::Stat::History History;
	
	public:
	
		explicit SpeedStat(
				const History &history,
				Strategy::TradingLog &log)
			: m_history(history), m_log(log) {
			//...//
		}
	Strategy::TradingLog &m_log;
	
		bool IsRising() const {
			const bool isRising
				= m_history.front().vwapAsk < m_history.back().vwapAsk
				&& m_history.front().emaFast < m_history.back().emaFast
				&& m_history.front().emaSlow < m_history.back().emaSlow;
			if (!isRising) {
				return false;
			}
			const double delta
				= m_history.front().vwapAsk < m_history[1].vwapAsk
					&& m_history[1].vwapAsk < m_history.back().vwapAsk
				?	1.0/ m_history.front().vwapAsk * m_history.back().vwapAsk
				:	CalcDelta(
						m_history.front().vwapAsk,
						m_history[1].vwapAsk,
						m_history.back().vwapAsk);
			const bool result = delta >= 1.00015;
			m_log.Write(
				"SpeedRatio\trising\t%1% -> %2% -> %3% = %4$.10f => %5%",
				[&](TradingRecord &record) {
					record
						% m_history.front().vwapAsk
						% m_history[1].vwapAsk
						% m_history.back().vwapAsk
						% delta
						% (result ? "true" : "false");
				});
			return result;
		}

		bool IsFalling() const {
			const bool isFalling
				= m_history.front().vwapBid > m_history.back().vwapBid
				&& m_history.front().emaFast > m_history.back().emaFast
				&& m_history.front().emaSlow > m_history.back().emaSlow;
			if (!isFalling) {
				return false;
			}
			const double delta
				= m_history.front().vwapBid > m_history[1].vwapBid
					&& m_history[1].vwapBid > m_history.back().vwapBid
				?	1.0 / m_history.back().vwapBid * m_history.front().vwapBid
				:	CalcDelta(
						m_history.back().vwapBid,
						m_history[1].vwapBid,
						m_history.front().vwapBid);
			const bool result = delta >= 1.00015;
			m_log.Write(
				"SpeedRatio\tfalling\t%1% -> %2% -> %3% = %4$.10f => %5%",
				[&](TradingRecord &record) {
					record
						% m_history.front().vwapBid
						% m_history[1].vwapBid
						% m_history.back().vwapBid
						% delta
						% (result ? "true" : "false");
				});
			return result;
		}

	private:

		static double CalcDelta(double smaller, double middle, double larger)  {
			AssertLt(smaller, larger);
			const double first = smaller < middle
				?	1.0 / smaller * middle
				:	1.0 / middle * smaller;
			const double second = larger < middle
				?	1.0 / larger * middle
				:	1.0 / middle * smaller;
			return first * second;
		}
	
	private:

		const History &m_history;

	};

	class SpeedCalculator : private boost::noncopyable {

	public:

		explicit SpeedCalculator(SpeedSet &resultRef)
			: m_result(resultRef)
			, m_isActivated(false) {
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				typedef std::remove_reference<SpeedSet::value_type>::type I;
				m_result.fill(std::numeric_limits<I>::quiet_NaN());
#			endif
		}

		~SpeedCalculator() {
			if (!m_isActivated) {
				// Not filling, not rising.
				m_result.fill(0);
			} else {
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				foreach (const auto &i, m_result) {
					Assert(!isnan(i));
				}
#			endif
			}
		}
	
		void operator ()(
				const Speed &type,
				double current,
				double prev) {
			Assert(isnan(m_result[type]));
			Assert(!IsEqual(current, prev));
			m_result[type] = current > prev
				?	((1.0 / prev) * current) - 1
				:	-(((1.0 / current) * prev) - 1);
			m_isActivated = true;
		}
	
	private:
	
		SpeedSet &m_result;
		bool m_isActivated;
	
	};

}

bool TriangulationWithDirection::CalcSpeed(PairsSpeed &result) const {

	typedef StatService::Stat::History History;
	typedef StatService::Point Point;

	for (size_t pair = 0; pair < result.size(); ++pair) {
			
		const History &data = m_bestBidAsk[pair].service->GetStat().history;
		//! @sa TRDK-240
		// Takes control over speed value, default value too so shuld be created
		// before first "continue", "break" or "return":
		SpeedCalculator calcSpeed(result[pair]);

		Assert(
			!(data.back().theo > data.back().emaFast
				&& data.back().emaFast > data.back().emaSlow)
			|| !(data.back().theo < data.back().emaFast
					&& data.back().emaFast < data.back().emaSlow));
		Assert(
			!(data.back().theo < data.back().emaFast
				&& data.back().emaFast < data.back().emaSlow)
			|| !(data.back().theo > data.back().emaFast
					&& data.back().emaFast > data.back().emaSlow));

		//! @sa TRDK-240
		AssertLe(2, data.size());
		boost::tribool isRising(boost::indeterminate);
		{
			const SpeedStat stat(data, GetTradingLog());
			if (stat.IsRising()) {
				isRising = true;
			} else if (stat.IsFalling()) {
				isRising = false;
			}
		}

		// theo-test
		if (isRising) {
			if (
					!(data.back().theo > data.back().emaFast
			  			&& data.back().emaFast > data.back().emaSlow)) {
				continue;
			}
		} else if (!isRising) {
			if (
					!(data.back().theo < data.back().emaFast
 					&& data.back().emaFast < data.back().emaSlow)) {
				continue;
			}
		} else {
			continue;
		}

		static_assert(numberOfSpeeds == 3, "List changed.");
		if (isRising) {
			calcSpeed(SPEED_VWAP, data.back().vwapAsk, data.front().vwapAsk);
		} else {
			calcSpeed(SPEED_VWAP, data.back().vwapBid, data.front().vwapBid);
		}
		calcSpeed(SPEED_EMA_FAST, data.back().emaFast, data.front().emaFast);
		calcSpeed(SPEED_EMA_SLOW, data.back().emaSlow, data.front().emaSlow);

	}

	return true;

}

namespace {

	class SpeedComparator : private boost::noncopyable {
	
	public:
	
		explicit SpeedComparator(const PairsSpeed &speedRef) 
			: m_speed(speedRef) {
			//...//
		}
	
		bool HasAnySpeed(const Pair &pair) const {
			return !IsZero(m_speed[pair].front());
		}
	
		bool IsRising(const Pair &pair) const {
			return m_speed[pair].front() > 0;
		
		}
	
		bool IsFalling(const Pair &pair) const {
			return m_speed[pair].front() < 0;

		}
	
		bool IsPair1Slower(const Pair &pair1, const Pair &pair2) const {

			for (size_t i = 0; i < numberOfSpeeds; ++i) {
				if (m_speed[pair1][i] >= m_speed[pair2][i]) {
					return false;
				}
			}

			return true;

		}

		bool IsPair1Faster(const Pair &pair1, const Pair &pair2) const {

			for (size_t i = 0; i < numberOfSpeeds; ++i) {
				if (fabs(m_speed[pair1][i]) <= fabs(m_speed[pair2][i])) {
					return false;
				}
			}

			return true;

		}
	
	private:
	
		const PairsSpeed &m_speed;
	
	};

}

bool TriangulationWithDirection::DetectByY1(Detection &result) const {

	AssertLe(1.0, m_yDetected[Y1]);

	const SpeedComparator comparator(result.speed);

	if (
			comparator.IsFalling(PAIR_AB)
			&& comparator.IsRising(PAIR_BC)
			&& !comparator.HasAnySpeed(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Slower(PAIR_AB, PAIR_BC)) {

		result.y = Y1;
		result.legs = {LEG1, LEG3, LEG2};

		return true;

	} else if (
			!comparator.HasAnySpeed(PAIR_AB)
			&& comparator.IsRising(PAIR_BC)
			&& comparator.IsFalling(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Faster(PAIR_BC, PAIR_AC)) {

		result.y = Y1;
		result.legs = {LEG2, LEG3, LEG1};

		return true;

	} else if (
			comparator.IsRising(PAIR_AB)
			&& comparator.IsFalling(PAIR_BC)
			&& !comparator.HasAnySpeed(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Faster(PAIR_AB, PAIR_BC)) {

		result.y = Y1;
		result.legs = {LEG3, LEG1, LEG2};

		return true;

	}

	return false;

}

bool TriangulationWithDirection::DetectByY2(Detection &result) const {

	AssertLe(1.0, m_yDetected[Y2]);

	const SpeedComparator comparator(result.speed);

	if (
			comparator.IsRising(PAIR_AB)
			&& comparator.IsFalling(PAIR_BC)
			&& !comparator.HasAnySpeed(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Slower(PAIR_AB, PAIR_BC)) {

		result.y = Y2;
		result.legs = {LEG1, LEG3, LEG2};

		return true;

	} else if (
			!comparator.HasAnySpeed(PAIR_AB)
			&& comparator.IsFalling(PAIR_BC)
			&& comparator.IsFalling(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Faster(PAIR_BC, PAIR_AC)) {

		result.y = Y2;
		result.legs = {LEG2, LEG3, LEG1};

		return true;

	} else if (
			comparator.IsFalling(PAIR_AB)
			&& comparator.IsRising(PAIR_BC)
			&& !comparator.HasAnySpeed(PAIR_AC)
			//! @sa TRDK-162 about falling/rising speed:
			&& comparator.IsPair1Faster(PAIR_AB, PAIR_BC)) {

		result.y = Y2;
		result.legs = {LEG3, LEG1, LEG2};

		return true;

	}

	return false;

}

bool TriangulationWithDirection::Detect(Detection &result) const {
	
	AssertNe(Y_UNKNOWN, CheckOpportunity(m_yDetected));
	
	if (!CalcSpeed(result.speed)) {
		return false;
	}
	
	if (m_yDetected[Y1] >= 1.0 && m_yDetected[Y2] <= m_yDetected[Y1]) {
		if (
				DetectByY1(result)
				|| (m_yDetected[Y2] >= 1.0 && DetectByY2(result))) {
			return true;
		}
	} else if (m_yDetected[Y2] >= 1.0) {
		if (
				DetectByY2(result)
				|| (m_yDetected[Y1] >= 1.0 && DetectByY1(result))) {
			return true;
		}
	} else if (m_yDetected[Y1] >= 1.0 && DetectByY1(result)) {
		return true;
	}

	Assert(m_yDetected[Y1] < 1.0 || !DetectByY1(result));
	Assert(m_yDetected[Y2] < 1.0 || !DetectByY2(result));
	return false;

}

size_t TriangulationWithDirection::CalcBookUpdatesNumber() const {
	size_t result = 0;
	foreach (const auto &bestBidAsk, m_bestBidAsk) {
		result += bestBidAsk.service->GetStat().numberOfUpdates;
	}
	return result;
}

void TriangulationWithDirection::CheckNewTriangle(
		const TimeMeasurement::Milestones &timeMeasurement) {

	Assert(!m_triangle);

 	if (CheckOpportunity(m_yDetected) == Y_UNKNOWN) {
		timeMeasurement.Measure(
			TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION_1);
 		return;
 	}

	timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START_1);

	Detection detection;
	if (!Detect(detection)) {
		return;
	}

	std::unique_ptr<Triangle> triangle;

	try {

		triangle.reset(
			new Triangle(
				m_generateUuid(),
				*this,
				m_reports,
				detection.y,
				m_qty,
				{
					PAIR_AB,
					detection.legs[PAIR_AB],
					detection.y == Y2,
					m_detectedEcns[detection.y][PAIR_AB]
				},
				{
					PAIR_BC,
					detection.legs[PAIR_BC],
					detection.y == Y2,
					m_detectedEcns[detection.y][PAIR_BC]
				},
				{
					PAIR_AC,
					detection.legs[PAIR_AC],
					detection.y == Y1,
					m_detectedEcns[detection.y][PAIR_AC],
				},
				m_bestBidAsk));

		triangle->StartLeg1(
			timeMeasurement,
			detection.speed,
			m_prevTriangle ? &*m_prevTriangle : nullptr);

	} catch (const HasNotMuchOpportunityException &ex) {
		GetLog().Warn(
			"Failed to start triangle: \"%1%\". Required: %2% %3%.",
			ex,
			ex.GetRequiredQty(),
			ex.GetSecurity());
		return;
	} catch (const PriceNotChangedException &) {
		GetTradingLog().Write(
			"\twaiting for changes to execute leg\t%1%",
			[this](TradingRecord &record) {
				record % GetLegNo(LEG1);
			});
		return;
	}

	Assert(!m_triangle);
	triangle.swap(m_triangle);

	return;

}

bool TriangulationWithDirection::CheckTriangleCompletion(
		const TimeMeasurement::Milestones &timeMeasurement) {
	
	Assert(m_triangle);

	if (
			!m_triangle->IsLegExecuted(LEG1)
			||	!m_triangle->IsLegExecuted(LEG2)
			||	m_triangle->IsLegStarted(LEG3)) {
		return false;
	}

	Triangle::PairInfo &leg3Info = m_triangle->GetPair(LEG3);
	const auto &data = leg3Info.bestBidAsk->service->GetStat();
	if (!IsProfit(leg3Info, data)) {
		timeMeasurement.Measure(
			TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION_2);
		return false;
	}

	Lib::TimeMeasurement::PeriodFromStart orderDelay;
	try {
		orderDelay = m_triangle->StartLeg3(timeMeasurement, false);
	} catch (const HasNotMuchOpportunityException &ex) {
		m_scheduledLeg = LEG3;
		GetLog().Warn(
			"Failed to start leg 3: \"%1%\"."
				" Required: %2% %3%."
				" Scheduled.",
			ex,
			ex.GetRequiredQty(),
			ex.GetSecurity());
		return false;
	}

	m_triangle->GetReport()
		.ReportAction("detected", "signal", LEG3, orderDelay);

	return true;

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
		case PLT_PROFIT_WAIT:
			return true;
		default:
			return false;
	}

	if (isJustOpened) {
		m_triangle->GetReport().ReportAction(
			"executed",
			"exec report",
			firstLeg.GetLeg(),
			0,
			&firstLeg);
	}

	m_triangle->Cancel(closeType);
	m_triangle->GetReport().ReportAction(
		"canceling",
		closeTypeStr,
		firstLeg.GetLeg(),
		0);

	return true;

}

TriangulationWithDirection::ProfitLossTest TriangulationWithDirection::CheckLeg(
		const Twd::Position &leg)
		const {

	if (!m_allowLeg1Closing) {
		return PLT_NONE;
	}

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

		const auto &data = m_bestBidAsk[leg.GetPair()].service->GetStat();
		if (IsProfit(m_triangle->GetPair(leg.GetLeg()), data)) {
			GetTradingLog().Write(
				"\tprofit\t%1%\t%2%\topp.: %3%\t%4%\tVWAP: %5%/%6%\tEMA slow: %7%",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record
						% data.history.back().vwapBid
						% data.history.back().vwapAsk
						% data.history.back().emaSlow;
				});
			return PLT_PROFIT;
		}


		GetTradingLog().Write(
			"\tprofit-detected\t%1%\t%2%\topp.: %3%\t%4%"
				"\t%5% -> %6%  = %7$.7f",
			[&](TradingRecord &record) {
				printTradingRecordStart(record);
				record
					% openPrice
					% currentPrice
					% seenProfit;
			});
		return PLT_PROFIT_WAIT;
	}
	

	return PLT_NONE;

}

void TriangulationWithDirection::OnCancel(
		const char *reason,
		const Twd::Position &reasonOrder,
		const TimeMeasurement::PeriodFromStart &delay) {
	Assert(m_triangle);
	m_triangle->GetReport().ReportAction(
		"canceled",
		reason,
		reasonOrder.GetLeg(),
		delay,
		&reasonOrder);
	m_prevTriangle.reset(m_triangle.release());
}

void TriangulationWithDirection::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);
	
	{
		const auto newTrianglesLimit = ReadMaxTrianglesCount(conf, GetLog());
		if (newTrianglesLimit > m_trianglesLimit) {
			m_trianglesRest += newTrianglesLimit - m_trianglesLimit;
		} else if (newTrianglesLimit < m_trianglesLimit) {
			if (m_trianglesLimit - newTrianglesLimit > m_trianglesRest) {
				m_trianglesRest = 0;
			} else {
				m_trianglesRest -= m_trianglesLimit - newTrianglesLimit;
			}
		}
		GetLog().Info(
			"Set triangles limit: %1% -> %2% (%3%).",
			m_trianglesLimit,
			newTrianglesLimit,
			m_trianglesRest);
		m_trianglesLimit = newTrianglesLimit;
	}

	{
		const auto newQty = conf.ReadTypedKey<Qty>("invest_amount");
		GetLog().Info("Set invest amount: %1% -> %2%.", m_qty, newQty);
		m_qty = newQty;
	}

}

void TriangulationWithDirection::OnPostionsCloseRequest() {
	
	if (!m_triangle) {
		GetLog().Info("Triangle not started, no opened positions.");
		return;
	}

	m_prevTriangle.reset();
	const std::unique_ptr<Triangle> triangle(m_triangle.release());

	try {

		if (m_scheduledLeg != LEG_UNKNOWN) {
			GetLog().Info(
				"Canceling scheduled leg %1%...",
				GetLegNo(m_scheduledLeg));
			m_scheduledLeg = LEG_UNKNOWN;
		}

		const auto cancelLeg = [this, &triangle](const Leg &leg) {
			if (!triangle->IsLegStarted(leg)) {
				return;
			}
			GetLog().Info("Canceling leg %1%...", GetLegNo(leg));
			triangle
				->GetLeg(leg)
				.CancelAtMarketPrice(Position::CLOSE_TYPE_REQUEST);
		};
		cancelLeg(LEG1);
		cancelLeg(LEG2);
		cancelLeg(LEG3);

	} catch (...) {
		AssertFailNoException();
		GetLog().Error("Failed to cancel triangle.");
		Block();
		throw;
	}

}

void TriangulationWithDirection::OnLeg1Execution(trdk::Position &position) {
	
	Assert(m_triangle);

	Twd::Position &order = dynamic_cast<Twd::Position &>(position);

	const auto &execDelay = position.GetTimeMeasurement().Measure(
		TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY_1);
	
	if (CheckCurrentStopRequest()) {
		m_triangle->GetReport().ReportAction(
			"executed",
			"exec report",
			LEG1,
			execDelay,
			&order);
		return;
	}
	
	if (CheckProfitLoss(order, true)) {
		return;
	}
	
	try {
		m_triangle->StartLeg2();
	} catch (const HasNotMuchOpportunityException &ex) {
		m_scheduledLeg = LEG2;
		GetLog().Warn(
			"Failed to start leg 2: \"%1%\"."
				" Required: %2% %3%."
				" Scheduled.",
			ex,
			ex.GetRequiredQty(),
			ex.GetSecurity());
		return;
	}
	
	m_triangle->GetReport().ReportAction(
		"executed",
		"exec report",
		"1 -> 2",
		execDelay,
		&order);

}

void TriangulationWithDirection::OnLeg2Execution(trdk::Position &position) {

	Assert(m_triangle);
	Assert(!m_triangle->GetLeg(LEG1).IsCompleted());

	Twd::Position &order = dynamic_cast<Twd::Position &>(position);
	AssertEq(LEG2, order.GetLeg());

	m_triangle->GetReport().ReportAction(
		"executed",
		"exec report",
		order.GetLeg(),
		0,
		&order);

	CheckCurrentStopRequest();

}

void TriangulationWithDirection::OnLeg3Execution(trdk::Position &position) {

	Assert(m_triangle);

	Twd::Position &order = dynamic_cast<Twd::Position &>(position);

	{
		const auto &execDelay = position.GetTimeMeasurement().Measure(
			TimeMeasurement::SM_STRATEGY_EXECUTION_REPLY_2);
		m_triangle->GetReport().ReportAction(
			"executed",
			"exec report",
			order.GetLeg(),
			execDelay,
			&order);
	}

	m_triangle->OnLeg3Execution();
	m_triangle->GetLeg(LEG1).MarkAsCompleted();
	m_triangle->GetLeg(LEG2).MarkAsCompleted();

	order.MarkAsCompleted();

	{
		const auto &y = m_triangle->CalcYExecuted();
		(y >= 1 ? m_stat.winners : m_stat.losers)(y);
	}
	
	m_prevTriangleTime = GetContext().GetCurrentTime();
	m_prevTriangle.reset(m_triangle.release());

#	ifdef DEV_VER
		m_detectedEcns[Y1].fill(std::numeric_limits<size_t>::max());
		m_detectedEcns[Y2].fill(std::numeric_limits<size_t>::max());
#	endif

	if (m_trianglesRest != nTrianglesLimit && m_trianglesRest-- <= 1) {
		throw RiskControlException("Executed triangles limit reached");
	} else {
		const auto winsCount = accs::count(m_stat.winners);
		const auto trianglesCount = winsCount + accs::count(m_stat.losers);
		const auto winsRatio = (winsCount * 100) / trianglesCount;
		GetContext().GetRiskControl(GetTradingMode()).CheckTotalWinRatio(
			GetRiskControlScope(),
			winsRatio,
			trianglesCount);
	}

	CheckCurrentStopRequest();

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
