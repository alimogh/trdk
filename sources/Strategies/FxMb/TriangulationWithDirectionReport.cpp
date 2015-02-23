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

namespace accs = boost::accumulators;
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
			Base::Write(formatCallback);
		}

	};

}

////////////////////////////////////////////////////////////////////////////////

class ReportsState::Startegy : private boost::noncopyable {

public:

	std::ofstream file;
	ReportLog log;

	explicit Startegy(Context &context, bool iEnabled) {

		if (!iEnabled) {
			return;
		}

		const pt::ptime &now = context.GetStartTime();
		boost::format fileName("s_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.log");
		fileName
			% now.date().year()
			% now.date().month().as_number()
			% now.date().day()
			% now.time_of_day().hours()
			% now.time_of_day().minutes()
			% now.time_of_day().seconds();
		const auto &logPath
			= context.GetSettings().GetLogsDir()
				/ "strategy"
				/ fileName.str();
		
		context.GetLog().Info("Triangles log: %1%.", logPath);
		fs::create_directories(logPath.branch_path());
		file.open(logPath.string().c_str(), std::ios::app | std::ios::ate);
		if (!file) {
			throw ModuleError("Failed to open triangles log file");
		}

		log.EnableStream(file);
	
	}

	void WriteHead(
			const Context &context,
			const BestBidAskPairs &bestBidAskPairs) {
		log.Write(
			[&context, &bestBidAskPairs](ReportRecord &record) {
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
					%	"Y targeted";
				foreach (const auto &bestBidAsk, bestBidAskPairs) {
					const char *pair
						= bestBidAsk
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
	}

};

class ReportsState::Pnl : private boost::noncopyable {

public:

	std::ofstream file;
	ReportLog log;

	const double comission;

	struct Stat {
	
		double comission;
		accs::accumulator_set<
				pt::time_duration,
				accs::stats<accs::tag::mean>>
			time;
	
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			winners;
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			winnersCancels;
		accs::accumulator_set<
				pt::time_duration,
				accs::stats<accs::tag::mean>>
			winnersTime;
		double winnersPnl;
	
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			losers;
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			losersCancels;
		accs::accumulator_set<
				pt::time_duration,
				accs::stats<accs::tag::mean>>
			losersTime;
		double losersPnl;
	
		explicit Stat()
			: comission(.0),
			winnersPnl(.0),
			losersPnl(.0) {
		}
	
	} stat;

	Pnl(Context &context, bool iEnabled, double comission)
		: comission(comission) {
	
		if (!iEnabled) {
			return;
		}

		const auto &logPath = context.GetSettings().GetLogsDir() / "pnl.log";
		context.GetLog().Info("PnL log: %1%.", logPath);
			
		const bool isNewFile = !fs::exists(logPath);
		if (isNewFile) {
			fs::create_directories(logPath.branch_path());
		}
			
		file.open(logPath.string().c_str(), std::ios::app | std::ios::ate);
		if (!file) {
			throw ModuleError("Failed to open PnL log file");
		}
		
		log.EnableStream(file);

		if (isNewFile) {
			WriteHead();
		}
	
	}

	void WriteHead() {
		log.Write(
			[](ReportRecord &record) {
				record
					%	"Date and logs #"
					%	"Triangle ID (winner)"
					%	"Winners"
					%	"Triangle ID (loser)"
					%	"Losers"
					%	"Triangle time"
					%	"Avg winners"
					%	"Avg winners time"
					%	"Avg losers"
					%	"Avg losers time"
					%	"# of winners"
					%	"# of losers"
					%	"% of winners"
					%	"Avg time"
					%	"Cancel ID (winner)"
					%	"Winners"
					%	"Cancel ID (loser)"
					%	"Losers"
					%	"Avg winners"
					%	"Avg losers"
					%	"# of winners"
					%	"# of losers"
					%	"P & L with commissions"
					%	"P & L without commissions"
					%	"Commission";
			});
	}

};

ReportsState::ReportsState(
		Context &context,
		double comission,
		bool enableStrategyLog,
		bool enablePriceUpdates,
		bool enablePnlLog)
	: strategy(new Startegy(context, enableStrategyLog)),
	pnl(new Pnl(context, enablePnlLog, comission)),
	enablePriceUpdates(enablePriceUpdates) {
}

void ReportsState::WriteStrategyLogHead(
		const Context &context,
		const BestBidAskPairs &bestBidAsk) {
	strategy->WriteHead(context, bestBidAsk);
}

ReportsState::~ReportsState() {
	delete pnl;
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
			% GetLegNo(info.leg)
			% (info.isBuy ? "buy" : "sell");

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
			%	info.bestBidAsk->bestBid.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.bestBidAsk->bestBid.source)
					.GetTag()
			%	info.bestBidAsk->bestAsk.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.bestBidAsk->bestAsk.source)
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
		const auto &data = info.bestBidAsk->service->GetData(
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

	double yExecuted = .0;
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

	m_state.strategy->log.Write(
		[&](ReportRecord &record) {
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
			if (isTriangleCompleted) {
				record % m_triangle.CalcYTargeted();
			} else {
				record % ' ';
			}
			for (size_t i = 0; i < numberOfPairs; ++i) {
				writePair(Pair(i), record);
			}
		});


	const auto &writePnl = [&](ReportRecord &record) {
		const auto pnl
			= m_state.pnl->stat.winnersPnl + m_state.pnl->stat.losersPnl;
		record
			% (pnl - m_state.pnl->stat.comission)
			% pnl
			% m_state.pnl->stat.comission;
	};
		
	if (isTriangleCompleted) {
			
		m_state.pnl->log.Write(
			[&](ReportRecord &record) {

				AssertGt(
					m_triangle.GetLeg(LEG3).GetOpenTime(),
					m_triangle.GetStartTime());
				const auto &time
					= m_triangle.GetLeg(LEG3).GetOpenTime()
						- m_triangle.GetStartTime();

				m_state.pnl->stat.comission += m_state.pnl->comission * 3;
				m_state.pnl->stat.time(time);

				const bool isWinner = yExecuted >= 1;
				if (isWinner) {
					m_state.pnl->stat.winners(yExecuted);
					m_state.pnl->stat.winnersPnl += yExecuted - 1;
					m_state.pnl->stat.winnersTime(time);
				} else {
					m_state.pnl->stat.losers(yExecuted);
					m_state.pnl->stat.losersPnl += yExecuted - 1;
					m_state.pnl->stat.losersTime(time);
				}

				record % ' ';

				if (!isWinner) {
					record % ' ' % ' ';
				}
				record % m_triangle.GetId() % yExecuted;
				if (isWinner) {
					record % ' ' % ' ';
				}
				record % time; 

				size_t winningPercentage = 0;
				if (accs::count(m_state.pnl->stat.winners) > 0) {
					record 
						% accs::mean(m_state.pnl->stat.winners)
						% accs::mean(m_state.pnl->stat.winnersTime);
					winningPercentage
						= (accs::count(m_state.pnl->stat.winners) * 100);
					winningPercentage
						/= accs::count(m_state.pnl->stat.winners)
							+ accs::count(m_state.pnl->stat.losers);
				} else {
					record % ' ' % ' ';
				}
				if (accs::count(m_state.pnl->stat.losers) > 0) {
					record
						% accs::mean(m_state.pnl->stat.losers)
						% accs::mean(m_state.pnl->stat.losersTime);
				} else {
					record % ' ' % ' ';
				}
				record
					% accs::count(m_state.pnl->stat.winners)
					% accs::count(m_state.pnl->stat.losers)
					% winningPercentage % '\0' % '%'
					% accs::mean(m_state.pnl->stat.time);

				for (auto i = 0; i < 8; ++i) {
					record % ' ';
				}

				writePnl(record);

			});

	} else if (isTriangleCanceled) {

		m_state.pnl->log.Write(
			[&](ReportRecord &record) {

				m_state.pnl->stat.comission += m_state.pnl->comission * 2;
			
				const bool isWinner = yExecuted >= 1;
				if (isWinner) {
					m_state.pnl->stat.winnersCancels(yExecuted);
					m_state.pnl->stat.winnersPnl += yExecuted - 1;
				} else {
					m_state.pnl->stat.losersCancels(yExecuted);
					m_state.pnl->stat.losersPnl += yExecuted - 1;
				}

				{
					const auto skipCount = isWinner ? 9 : 9 + 2;
					for (auto i = 0; i < skipCount; ++i) {
						record % ' ';
					}
				}

				record % m_triangle.GetId() % yExecuted;

				if (isWinner) {
					record % ' ' % ' ';
					record % accs::mean(m_state.pnl->stat.winnersCancels);
					record % ' ';
					record % accs::count(m_state.pnl->stat.winnersCancels);
					record % ' ';
				} else {
					record % ' ' % accs::mean(m_state.pnl->stat.losersCancels);
					record % ' ' % accs::count(m_state.pnl->stat.losersCancels);
				}

				writePnl(record);

			});

	}

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
			record
				% order->GetPlanedQty()
				% ConvertToIsoPch(order->GetCurrency());
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
			%	info.bestBidAsk->bestBid.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.bestBidAsk->bestBid.source)
					.GetTag()
			%	info.bestBidAsk->bestAsk.price
			%	m_triangle
					.GetStrategy()
					.GetContext()
					.GetMarketDataSource(info.bestBidAsk->bestAsk.source)
					.GetTag();

		// Rising/falling speed: ///////////////////////////////////////////////////////
		record % ' ' % ' ';
		
		// Stat data: //////////////////////////////////////////////////////////////////
		const auto &data = info.bestBidAsk->service->GetData(
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
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y1]
				% m_triangle.GetStrategy().GetYDetectedDirection()[Y2]
				% m_triangle.GetYDirection()[Y1]
				% m_triangle.GetYDirection()[Y2]
				% ' '
				% ' ';
			for (size_t i = 0; i < numberOfPairs; ++i) {
				writePair(Pair(i), record);
			}
		});

}


