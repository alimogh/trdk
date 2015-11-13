/**************************************************************************
 *   Created: 2015/01/18 11:08:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirection.hpp"
#include "TriangulationWithDirectionTriangle.hpp"
#include "TriangulationWithDirectionReport.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/AsyncLog.hpp"
#include "Core/Context.hpp"
#include "Core/Settings.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class ReportRecord : public AsyncLogRecord {

	public:

		explicit ReportRecord(
				const Log::Time &time,
				const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}

	public:

		const ReportRecord & operator >>(std::ostream &os) const {
			Dump(os, "\t");
			return *this;
		}

	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const ReportRecord &record) {
		record >> os;
		return os;
	}

	class ReportLogOutStream : private boost::noncopyable {
	public:
		void Write(const ReportRecord &record) {
			m_log.Write(record);
		}
		bool IsEnabled() const {
			return m_log.IsEnabled();
		}
		void EnableStream(std::ostream &os) {
			m_log.EnableStream(os, false);
		}
		Log::Time GetTime() {
			return std::move(m_log.GetTime());
		}
		Log::ThreadId GetThreadId() const {
			return std::move(m_log.GetThreadId());
		}
	private:
		Log m_log;
	};

	typedef trdk::AsyncLog<
			ReportRecord,
			ReportLogOutStream,
			TRDK_CONCURRENCY_PROFILE>
		ReportLogBase;

	class ReportLog : private ReportLogBase {

	public:

		typedef ReportLogBase Base;

	public:

		using Base::IsEnabled;
		using Base::EnableStream;

		template<typename FormatCallback>
		void Write(const FormatCallback &formatCallback) {
			Base::FormatAndWrite(formatCallback);
		}

	};

}

////////////////////////////////////////////////////////////////////////////////

class ReportsState::Strategy : private boost::noncopyable {

public:

	std::ofstream file;
	ReportLog log;

	explicit Strategy(
			Context &context,
			const boost::uuids::uuid &strategyId,
			bool iEnabled) {

		if (!iEnabled) {
			return;
		}

		const pt::ptime &now = context.GetStartTime();
		boost::format fileName("%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.log");
		fileName
			% now.date().year()
			% now.date().month().as_number()
			% now.date().day()
			% now.time_of_day().hours()
			% now.time_of_day().minutes()
			% now.time_of_day().seconds();
		const auto &logPath
			= context.GetSettings().GetLogsDir()
				/ boost::lexical_cast<std::string>(strategyId)
				/ fileName.str();
		
		context.GetLog().Info("Triangles log: %1%.", logPath);
		fs::create_directories(logPath.branch_path());
		file.open(logPath.string().c_str(), std::ios::app | std::ios::ate);
		if (!file) {
			throw ModuleError("Failed to open triangles log file");
		}

		log.EnableStream(file);
	
	}

	void WriteHead(const Context &context, const PairsData &allPairsData) {
		log.Write(
			[&context, &allPairsData](ReportRecord &record) {
				record
					%	"No"
					%	"Time"
					%	"Action"
					%	"Action reason"
					%	"Action legs"
					%	"Action orders count"
					%	"Last Orders ID"
					%	"Delay"
					%	"Y1 detected"
					%	"Y2 detected"
					%	"Y executed"
					%	"Y targeted"
					%	"ATR";
				foreach (const auto &pairData, allPairsData) {
					const char *pair
						= pairData
							.service
							->GetSecurity(0)
							.GetSymbol()
							.GetSymbol()
							.c_str();
					record
						%	pair % '\0' % " ECN"
						%	pair % '\0' % " leg no."
						%	pair % '\0' % " direction"
						%	pair % '\0'	% " order qty"
						%	pair % '\0' % " order state" 
						%	pair % '\0' % " price start"
						%	pair % '\0' % " price exec"
						%	pair % '\0' % " bid"
						%	pair % '\0' % " ask"
						%	pair % '\0' % " best bid"
						%	pair % '\0' % " best bid ECN"
						%	pair % '\0' % " best ask"
						%	pair % '\0' % " best ask ECN"
						%	pair % '\0' % " rising speed"
						%	pair % '\0' % " falling speed"
						%	pair % '\0' % " VWAP"
						%	pair % '\0' % " VWAP prev1"
						%	pair % '\0' % " VWAP prev2"
						%	pair % '\0' % " EMA fast"
						%	pair % '\0' % " EMA fast prev1"
						%	pair % '\0' % " EMA fast prev2"
						%	pair % '\0' % " EMA slow"
						%	pair % '\0' % " EMA slow prev1"
						%	pair % '\0' % " EMA slow prev2";
				}
				record % "Start: " % '\0' % context.GetStartTime();
				{
					const auto &utc = pt::microsec_clock::universal_time();
					record
						% "UTC: " % '\0' % utc
						% "EST: " % '\0' % (utc + GetEstDiff());
				}
				record
					% "Build: " TRDK_BUILD_IDENTITY
					% "Build time: " __DATE__ " " __TIME__;
			});
// 	log.Write(
// 		[&context, &allPairsData](ReportRecord &record) {
// 			record
// 				%	"No"
// 				%	"Time"
// 				%	"Action"
// 				%	"Y1"
// 				%	"Y2"
// 				%	"Y1 or Y2";
// 			foreach (const auto &pairData, allPairsData) {
// 				const char *pair
// 					= pairData
// 						.service
// 						->GetSecurity(0)
// 						.GetSymbol()
// 						.GetSymbol()
// 						.c_str();
// 				record
// 					%	pair % '\0' % " leg no."
// 					%	pair % '\0' % " direction"
// 					%	pair % '\0' % " bid at start"
// 					%	pair % '\0' % " ask at start"
// 					%	pair % '\0' % " bid triangulation"
// 					%	pair % '\0' % " ask triangulation"
// 					%	pair % '\0' % " qty"
// 					%	pair % '\0' % " tag 15"
// 					%	pair % '\0' % " tag 54";
// 			}
// 		});

	}

};

ReportsState::ReportsState(
		Context &context,
		const boost::uuids::uuid &strategyId,
		bool enableStrategyLog,
		bool enablePriceUpdates)
	: strategy(new Strategy(context, strategyId, enableStrategyLog)),
	enablePriceUpdates(enablePriceUpdates) {
}

void ReportsState::WriteStrategyLogHead(
		const Context &context,
		const PairsData &pairData) {
	strategy->WriteHead(context, pairData);
}

ReportsState::~ReportsState() {
	delete strategy;
}

////////////////////////////////////////////////////////////////////////////////

void TriangleReport::ReportAction(
		const char *action,
		const char *reason,
		const Leg &actionLeg,
		const TimeMeasurement::PeriodFromStart &delay,
		const Twd::Position *const reasonOrder /*= nullptr*/) {
	const char *actionLegStr;
	switch (actionLeg) {
		case LEG1:
			actionLegStr = "1";
			break;
		case LEG2:
			actionLegStr = "2";
			break;
		case LEG3:
			actionLegStr = "3";
			break;
		default:
			actionLegStr = "unknown";
			AssertEq(1, actionLeg);
			break;
	}
	ReportAction(action, reason, actionLegStr, delay, reasonOrder);
}


void TriangleReport::ReportAction(
		const char *action,
		const char *reason,
		const char *actionLegs,
		const TimeMeasurement::PeriodFromStart &delay,
		const Twd::Position *const reasonOrder /*= nullptr*/,
		const PairsSpeed *speed /*= nullptr*/) {

	const auto &writePair = [&](const Pair &pair, ReportRecord &record) {
		
		const Triangle::PairInfo &info = m_triangle.GetPair(pair);
		const Twd::Position *const order = m_triangle.IsLegStarted(info.leg)
			?	&m_triangle.GetLeg(info.leg)
			:	nullptr;
		const Security &security = m_triangle.GetCalcSecurity(pair);

		record
			% security.GetSource().GetTag()
			% GetLegNo(info.leg);

		record % (info.isBuy ? "buy" : "sell");

		if (!order) {
			record % ' ';
		} else {
			record % order->GetPlanedQty();
		}
		
		if (!order) {
			record % "wait";
		} else if (order->IsClosed()) {
			record % "closed";
		} else if (!IsZero(order->GetCloseStartPrice())) {
			AssertLt(0, order->GetOpenStartPrice());
			record % "closing";
		} else if (order->IsOpened()) {
			record % "opened";
		} else if (!order->HasActiveOrders()) {
			record % "canceled";
		} else {
			record % "opening";
		}

		if (!order) {
			record % ' ' % ' ';
		} else if (!IsZero(order->GetCloseStartPrice())) {
			const Security &security = order->GetSecurity();
			record % security.DescalePrice(order->GetCloseStartPrice());
			if (order->IsClosed()) {
				record % security.DescalePrice(order->GetClosePrice());
			} else {
				record % ' ';
			}
		} else {
			record % info.startPrice;
			if (order->IsOpened()) {
				record
					% order->GetSecurity().DescalePrice(order->GetOpenPrice());
			} else {
				record % ' ';
			}
		}

		// Chosen ECN bid/ask:  ///////////////////////////////////////////////////////
		record 
			%	security.GetBidPrice()
			%	security.GetAskPrice();
		
		// Best bid/ask and ECNs: ////////////////////////////////////////////////////
		record
			%	info.pairData->bestBid.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.pairData->bestBid.source)
					.GetTag()
			%	info.pairData->bestAsk.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.pairData->bestAsk.source)
					.GetTag();

		// Rising/falling speed: ///////////////////////////////////////////////////////
		if (speed) {
			if (IsZero((*speed)[pair])) {
				record % ' ' % ' ';
			}  else if ((*speed)[pair] > 0) {
				record % (*speed)[pair] % ' ';
			} else {
				AssertGt(0, (*speed)[pair]);
				record % ' ' % fabs((*speed)[pair]);
			}
		} else {
			record % ' ' % ' ';
		}
		
		// Stat data: //////////////////////////////////////////////////////////////////
		const auto &data = info.pairData->service->GetData(
			security.GetSource().GetIndex());
		record
			%	data.current.theo
			%	data.prev1.theo
			%	data.prev2.theo
			%	data.current.emaFast
			%	data.prev1.emaFast
			%	data.prev2.emaFast
			%	data.current.emaSlow
			%	data.prev1.emaSlow
			%	data.prev2.emaSlow;

	};

	m_state.strategy->log.Write(
		[&](ReportRecord &record) {

			const bool isTriangleCanceled
				= reasonOrder
					&& reasonOrder->GetLeg() == LEG1
					&& reasonOrder->IsClosed();
			const bool isTriangleCompleted
				= reasonOrder
					&& reasonOrder->GetLeg() == LEG3
					&& reasonOrder->IsOpened();
			Assert(
				isTriangleCanceled != isTriangleCompleted
					|| !isTriangleCompleted);

			double yExecuted;
			if (isTriangleCompleted) {
				yExecuted = m_triangle.CalcYExecuted();
			} else if (isTriangleCanceled) {
				const Security &security = reasonOrder->GetSecurity();
				const auto &entryPrice
					= security.DescalePrice(reasonOrder->GetOpenPrice());
				const auto &exitPrice
					= security.DescalePrice(reasonOrder->GetClosePrice());
				yExecuted = (1 / entryPrice) * exitPrice;
			}

			record
				% m_triangle.GetId()
				% m_triangle.GetStrategy().GetContext().GetCurrentTime()
				% action
				% reason
				% actionLegs;
			if (reasonOrder) {
				record % m_triangle.GetPair(reasonOrder->GetLeg()).ordersCount;
				if (reasonOrder->IsClosed()) {
					record % reasonOrder->GetCloseOrderId();
				} else if (reasonOrder->IsOpened()) {
					record % reasonOrder->GetOpenOrderId();
				} else {
					record % ' ';
				}
			} else {
				record % ' ' % ' ';
			}
			if (delay != 0) {
				record % delay;
			} else {
				record % ' ';
			}
			record
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y1]
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y2];
			if (isTriangleCompleted || isTriangleCanceled) {
				record % yExecuted;
			} else {
				record % ' ';
			}
			record % m_triangle.CalcYTargeted();

			record % m_triangle.GetLastAtr();

			for (size_t i = 0; i < numberOfPairs; ++i) {
				writePair(Pair(i), record);
			}
		
		});

}

void TriangleReport::ReportUpdate() {

	if (!m_state.enablePriceUpdates) {
		return;
	}

	const auto &writePair = [&](const Pair &pair, ReportRecord &record) {
		
		const Triangle::PairInfo &info = m_triangle.GetPair(pair);
		const Twd::Position *const order = m_triangle.IsLegStarted(info.leg)
			?	&m_triangle.GetLeg(info.leg)
			:	nullptr;
		const Security &security = m_triangle.GetCalcSecurity(pair);

		record % security.GetSource().GetTag();

		record % GetLegNo(info.leg) % (info.isBuy ? "buy" : "sell");
		
		if (!order) {
			record % ' ' % ' ';
		} else {
			record % order->GetPlanedQty() % order->GetCurrency();
		}
		
		if (!order) {
			record % "wait";
		} else if (order->IsClosed()) {
			record % "closed";
		} else if (!IsZero(order->GetCloseStartPrice())) {
			AssertLt(0, order->GetOpenStartPrice());
			record % "closing";
		} else if (order->IsOpened()) {
			record % "opened";
		} else if (!order->HasActiveOrders()) {
			record % "canceled";
		} else {
			record % "opening";
		}

		if (!order) {
			record % ' ' % ' ';
		} else if (!IsZero(order->GetCloseStartPrice())) {
			const Security &security = order->GetSecurity();
			record % security.DescalePrice(order->GetCloseStartPrice());
			if (order->IsClosed()) {
				record % security.DescalePrice(order->GetClosePrice());
			} else {
				record % ' ';
			}
		} else {
			record % info.startPrice;
			if (order->IsOpened()) {
				record
					% order->GetSecurity().DescalePrice(order->GetOpenPrice());
			} else {
				record % ' ';
			}
		}

		// Chosen ECN bid/ask:  ///////////////////////////////////////////////////////
		record 
			%	security.GetBidPrice()
			%	security.GetAskPrice();
		
		// Best bid/ask and ECNs: ////////////////////////////////////////////////////
		record
			%	info.pairData->bestBid.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.pairData->bestBid.source)
					.GetTag()
			%	info.pairData->bestAsk.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.pairData->bestAsk.source)
					.GetTag();

		// Rising/falling speed: ///////////////////////////////////////////////////////
		record % ' ' % ' ';
		
		// Stat data: //////////////////////////////////////////////////////////////////
		const auto &data = info.pairData->service->GetData(
			security.GetSource().GetIndex());
		record
			%	data.current.theo
			%	data.prev1.theo
			%	data.prev2.theo
			%	data.current.emaFast
			%	data.prev1.emaFast
			%	data.prev2.emaFast
			%	data.current.emaSlow
			%	data.prev1.emaSlow
			%	data.prev2.emaSlow;

	};

	m_state.strategy->log.Write(
		[&](ReportRecord &record) {
			record
				% m_triangle.GetId()
				% m_triangle.GetStrategy().GetContext().GetCurrentTime()
				% "update"
				% ' '
				% ' '
				% ' '
				% ' '
				% ' '
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y1]
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y2]
				% ' '
				% ' '
				% m_triangle.GetLastAtr();
			for (size_t i = 0; i < numberOfPairs; ++i) {
				writePair(Pair(i), record);
			}
		});

}


