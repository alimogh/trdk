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
#include "TriangulationWithDirectionPosition.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

namespace trdk { namespace Strategies { namespace FxMb { namespace Twd {
	class TriangulationWithDirection;
} } } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class StrategyLogRecord : public AsyncLogRecord {
	public:
		explicit StrategyLogRecord(
				const Log::Time &time,
				const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}
	public:
		const StrategyLogRecord & operator >>(std::ostream &os) const {
			Dump(os, ",");
			return *this;
		}
	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const StrategyLogRecord &record) {
		record >> os;
		return os;
	}

	class StrategyLogOutStream : private boost::noncopyable {
	public:
		void Write(const StrategyLogRecord &record) {
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
			StrategyLogRecord,
			StrategyLogOutStream,
			TRDK_CONCURRENCY_PROFILE>
		StrategyLogBase;

	class StrategyLog : private StrategyLogBase {

	public:

		typedef StrategyLogBase Base;

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

class Strategies::FxMb::Twd::TriangulationWithDirection : public Strategy {
		
public:
		
	typedef Strategy Base;

private:

	typedef std::vector<ScaledPrice> BookSide;

	struct Stat {

		const StatService *service;
		
		struct {
			
			double price;
			size_t source;
		
			void Reset() {
				price = 0;
			}

		}
			bestBid,
			bestAsk;

		void Reset() {
			bestBid.Reset();
			bestAsk.Reset();
		}

	};

	typedef boost::array<boost::shared_ptr<Twd::Position>, numberOfPairs>
		Orders;

public:

	explicit TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, "TriangulationWithDirection", tag),
		m_levelsCount(
			conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
		m_qty(conf.ReadTypedKey<Qty>("qty")),
		m_opportunityReportStep(
			conf.ReadTypedKey<double>("opportunity_report_step")),
		m_opportunityNo(0) {

		{
			const Stat def = {};
			m_stat.fill(def);
		}
		m_opportunity.fill(.0);
		m_reportedOpportunity.fill(.0);
		
		if (conf.ReadBoolKey("log")) {
		
			const pt::ptime &now = GetContext().GetStartTime();
			boost::format fileName(
				"strategy_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.csv");
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
		
			GetContext().GetLog().Info("Log: %1%.", logPath);
			fs::create_directories(logPath.branch_path());
			m_strategyLogFile.open(
				logPath.string().c_str(),
				std::ios::app | std::ios::ate);
			if (!m_strategyLogFile) {
				throw ModuleError("Failed to open log file");
			}
			m_strategyLog.EnableStream(m_strategyLogFile);

		}

	}

	virtual ~TriangulationWithDirection() {
		//...//
	}

public:

	bool HasOpportunity() const {
		return 
			(m_opportunity[0] >= 1.0 && m_opportunity[1] > .0)
			|| (m_opportunity[1] >= 1.0 && m_opportunity[0] > .0);
	}

	bool IsActive() const {
		return m_orders[0] ? true : false;
	}

public:
		
	virtual void OnServiceStart(const Service &service) {

		const Stat statService = {
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
		Assert(!m_stat[index].service);
		m_stat[index] = statService;

		{
			bool isFull = true;
			foreach (const auto &s, m_stat) {
				if (!s.service) {
					isFull = false;
					break;
				}
			}
			if (isFull) {
				WriteLogHead();
			}
		}

// @todo see https://trello.com/c/ONnb5ai2
//		m_stat.push_back(statService);

	}

	virtual void OnServiceDataUpdate(const Service &service) {

		UpdateStat(service);

		if (IsActive()) {
			CheckClosePossibility();
		} else {
			CheckOpenPossibility();
		}

	}

	void OnPositionUpdate(trdk::Position &position) {

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
		
				case 1:
					OnCancel("exec report", order);
					break;
		
				case 2:
					{
						Twd::Position &firstLeg = GetLeg(1);
						Assert(!firstLeg.IsActive());
						Assert(IsZero(firstLeg.GetCloseStartPrice()));
						Assert(firstLeg.IsOpened());
						AssertLt(0, firstLeg.GetActiveQty());
						if (!CheckLeg(firstLeg)) {
							CloseLeg(firstLeg);
							LogAction(
								"canceling",
								"not executable",
								firstLeg.GetLeg());
						} else {
							ReplaceOrder(order, false);
						}
					}
					break;
		
				case 3:
					ReplaceOrder(order, true);
					break;
		
				default:
					AssertEq(1, order.GetLeg());
					AssertFail("Unknown leg no.");
					break;
			
			}
		
		} else if (!IsZero(order.GetCloseStartPrice())) {
			
			AssertLt(0, order.GetOpenedQty());
			AssertEq(1, order.GetLeg());
			
			if (order.GetActiveQty() == 0) {
				OnCancel("exec report", order, true, true);
			} else {
				CloseLeg(order);
			}
		
		} else {
 
 			Assert(order.IsOpened());
			Assert(IsZero(order.GetCloseStartPrice()));
			AssertEq(0, order.GetClosedQty());
			Assert(!order.HasActiveOrders());

			switch (order.GetLeg()) {
			
				case 1:
					if (!CheckLeg(order)) {
						LogAction(
							"executed",
							"exec report",
							order.GetLeg(),
							&order);
						CloseLeg(order);
						LogAction(
							"canceling",
							"not executable",
							order.GetLeg());
						return;
					}
					GetLeg(2).OpenAtStartPrice();
					LogAction("executed", "exec report", "1 -> 2", &order);
					break;

				case 3:
					LogAction(
						"executed",
						"exec report",
						order.GetLeg(),
						&order,
						true,
						true);
					m_orders.fill(boost::shared_ptr<Twd::Position>());
					break;

				default:
					LogAction(
						"executed",
						"exec report",
						order.GetLeg(),
						&order);
					break;
			
			}

		}

	}

private:

	//! Updating min/max and Y1/Y2.
	void UpdateStat(const Service &service) {
		
		auto stat = m_stat.begin();
		{
			const auto &end = m_stat.end();
			for ( ; stat != end && stat->service != &service; ++stat);
			Assert(stat != end);
		}

		stat->Reset();
		const auto &ecnsCount = GetContext().GetMarketDataSourcesCount();
		bool hasNotOpportunity = false;;
		for (size_t ecn = 0; !hasNotOpportunity && ecn < ecnsCount; ++ecn) {
			const Security &security = stat->service->GetSecurity(ecn);
			{
				const auto &bid = security.GetBidPrice();
				if (IsZero(bid)) {
					hasNotOpportunity = true;
				} else if (stat->bestBid.price < bid) {
					stat->bestBid.price = bid;
					stat->bestBid.source = ecn;
				}
			}
			{
				const auto &ask = security.GetAskPrice();
				if (IsZero(ask)) {
					hasNotOpportunity = true;
				} else if (
						stat->bestAsk.price > ask
						|| IsZero(stat->bestAsk.price)) {
					stat->bestAsk.price = ask;
					stat->bestAsk.source = ecn;
				}
			}
		}

		if (hasNotOpportunity) {
			
			stat->Reset();
			
			m_opportunity.fill(0);

		} else {


			Assert(!IsZero(stat->bestAsk.price));
			stat->bestAsk.price = 1.0 / stat->bestAsk.price;

			////////////////////////////////////////////////////////////////////////////////
			// Y1 and Y2:
			// 

			m_opportunity[0]
				= m_stat[PAIR_AB].bestBid.price
					* m_stat[PAIR_BC].bestBid.price
					* m_stat[PAIR_AC].bestAsk.price;
			m_opportunity[1]
				= m_stat[PAIR_AC].bestBid.price
					* m_stat[PAIR_BC].bestAsk.price
					* m_stat[PAIR_AB].bestAsk.price;

		}

		const auto &isTimeToReport = [this](double current, double reported) {
			return
				reported > 1.0 != current > 1.0
				|| current < (reported - m_opportunityReportStep)
				|| (reported + m_opportunityReportStep) < current;
		};
		if (
				isTimeToReport(m_opportunity[0], m_reportedOpportunity[0])
				|| isTimeToReport(m_opportunity[1], m_reportedOpportunity[1])) {
			GetTradingLog().Write(
				"\topportunity\tY1: %1% -> %2%\tY2: %3% -> %4%",
				[this](TradingRecord &record) {
					record
						% m_reportedOpportunity[0]
						% m_opportunity[0]
						% m_reportedOpportunity[1]
						% m_opportunity[1];
				});
			m_reportedOpportunity = m_opportunity;
		}

	}

	void CalcOpenPossibility() {

		Assert(HasOpportunity());

		////////////////////////////////////////////////////////////////////////////////
		// Detecting current rising / falling:

		const auto &getMovementSpeed = [this](const Pair &pair) -> double {
			const auto &stat = m_stat[pair];
			{
				const auto &statData
					= stat.service->GetData(stat.bestBid.source);
				const bool isRising
					= statData.current.theo > statData.current.emaFast
						&& statData.current.emaFast > statData.current.emaSlow;
				if (isRising) {
					const auto diff
						= statData.prev2.theo - statData.current.theo;
					return fabs(diff);
				}
			}
			{
				const auto &statData
					= stat.service->GetData(stat.bestAsk.source);
				const bool isFalling
					= statData.current.theo < statData.current.emaFast
						&& statData.current.emaFast < statData.current.emaSlow;
				if (isFalling) {
					const auto diff
						= statData.prev2.theo - statData.current.theo;
					return fabs(diff) * -1.0;
				}
			}
			return .0;
		};

		double movementSpeed[2] = {
			getMovementSpeed(PAIR_AB),
			getMovementSpeed(PAIR_AC)
		};
		if (IsZero(movementSpeed[0]) && IsZero(movementSpeed[1])) {
			m_opportunitySource.isRising.reset();
			return;
		}
		if (!IsZero(movementSpeed[0]) && !IsZero(movementSpeed[1])) {
			if (fabs(movementSpeed[0]) >= fabs(movementSpeed[1])) {
				movementSpeed[1] = .0;
			} else {
				movementSpeed[0] = .0;
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Orders
		
		if (!IsZero(movementSpeed[0])) {
			Assert(IsZero(movementSpeed[1]));
			m_opportunitySource.isRising = movementSpeed[0] > 0;
			m_opportunitySource.legsNo[0] = 1;
			m_opportunitySource.isBuy[0] = *m_opportunitySource.isRising;
			m_opportunitySource.legsNo[1] = 2;
			m_opportunitySource.isBuy[1] = !*m_opportunitySource.isRising;
		} else {
			Assert(!IsZero(movementSpeed[1]));
			m_opportunitySource.isRising = movementSpeed[1] > 0;
			m_opportunitySource.legsNo[0] = 2;
			m_opportunitySource.isBuy[0] = !*m_opportunitySource.isRising;
			m_opportunitySource.legsNo[1] = 1;
			m_opportunitySource.isBuy[1] = *m_opportunitySource.isRising;
		}

	}

	Twd::Position & GetLeg(size_t legNo) {
		return GetLeg(m_orders, legNo);
	}

	static Twd::Position & GetLeg(Orders &orders, size_t legNo) {
		foreach (const auto &leg, orders) {
			if (leg->GetLeg() == legNo) {
				return *leg;
			}
		}
		AssertNe(legNo, legNo);
		throw std::logic_error("Unknown Leg No");
	}

	void ReplaceOrder(Twd::Position &oldOrder, bool useCurrentPrice) {

		boost::shared_ptr<Position> newOrder;
		if (oldOrder.GetType() == Position::TYPE_LONG) {
			newOrder.reset(
				new Twd::LongPosition(
					*this,
					oldOrder.GetTradeSystem(),
					oldOrder.GetSecurity(),
					oldOrder.GetCurrency(),
					oldOrder.GetPlanedQty(),
					oldOrder.GetOpenStartPrice(),
					TimeMeasurement::Milestones(),
					oldOrder.GetPair(),
					oldOrder.GetLeg(),
					oldOrder.IsByRising(),
					oldOrder.GetOrdersCount()));
		} else {
			newOrder.reset(
				new Twd::ShortPosition(
					*this,
					oldOrder.GetTradeSystem(),
					oldOrder.GetSecurity(),
					oldOrder.GetCurrency(),
					oldOrder.GetPlanedQty(),
					oldOrder.GetOpenStartPrice(),
					TimeMeasurement::Milestones(),
					oldOrder.GetPair(),
					oldOrder.GetLeg(),
					oldOrder.IsByRising(),
					oldOrder.GetOrdersCount()));
		}

		foreach (auto &order, m_orders) {
			if (order->GetLeg() == newOrder->GetLeg()) {
				order = newOrder;
				if (useCurrentPrice) {
					order->OpenAtCurrentPrice();
				} else {
					order->OpenAtStartPrice();
				}
				return;
			}
		}

		AssertNe(oldOrder.GetLeg(), oldOrder.GetLeg());
		throw std::logic_error("Unknown Leg No from order");

	}

	void CheckOpenPossibility() {

 		if (!HasOpportunity()) {
 			return;
 		}

		CalcOpenPossibility();
		if (!m_opportunitySource.isRising) {
			return;
		}

		struct HasNotMuchOpportunity {
			const Security *security;
			const char *side;
			explicit HasNotMuchOpportunity(
					const Security &security,
					const char *side)
				: security(&security),
				side(side) {
				//...//
			}
		};

		const auto &newPosition = [&](
				const Pair &pair,
				size_t legNo,
				bool isRising,
				bool isBuy) 
				-> boost::shared_ptr<Twd::Position> {
			const auto &stat = m_stat[pair];
			const auto &ecn = isRising
				?	stat.bestBid.source
				:	stat.bestAsk.source;
			const Security &security = stat.service->GetSecurity(ecn);
			boost::shared_ptr<Twd::Position> result;
			if (isBuy) {
				if (security.GetAskQty() == 0) {
					throw HasNotMuchOpportunity(security, "ask");
				}
				result.reset(
					new Twd::LongPosition(
						*this,
						GetContext().GetTradeSystem(ecn),
						//! @todo fixme:
						const_cast<Security &>(security),
						security.GetSymbol().GetCashBaseCurrency(),
						m_qty,
						security.GetAskPriceScaled(),
						TimeMeasurement::Milestones(),
						pair,
						legNo,
						isRising,
						0));
			} else {
				if (security.GetBidQty() == 0) {
					throw HasNotMuchOpportunity(security, "bid");
				}
				result.reset(
					new Twd::ShortPosition(
						*this,
						GetContext().GetTradeSystem(ecn),
						//! @todo fixme:
						const_cast<Security &>(security),
						security.GetSymbol().GetCashBaseCurrency(),
						m_qty,
						security.GetBidPriceScaled(),
						TimeMeasurement::Milestones(),
						pair,
						legNo,
						isRising,
						0));
			}
			return std::move(result);
		};

		try {

			Orders orders = {
				newPosition(
					PAIR_AB,
					m_opportunitySource.legsNo[0],
					*m_opportunitySource.isRising,
					m_opportunitySource.isBuy[0]),
				newPosition(
					PAIR_BC,
					3,
					*m_opportunitySource.isRising,
					m_opportunitySource.isBuy[0]),
				newPosition(
					PAIR_AC,
					m_opportunitySource.legsNo[1],
					!*m_opportunitySource.isRising,
					m_opportunitySource.isBuy[1])
			};
			GetLeg(orders, 1).OpenAtStartPrice();
			orders.swap(m_orders);

			++m_opportunityNo;
			LogAction("detected", "signal", "1");

		} catch (const HasNotMuchOpportunity &ex) {
			GetTradingLog().Write(
				"\tskipped decision\t"
					" \"%1%\" has not much opportunity at \"%2%\" (%3%).",
				[&ex](TradingRecord &record) {
					record
						% *ex.security
						% ex.security->GetSource().GetTag()
						% ex.side;
				});
		}

	}

	void CheckClosePossibility() {

		foreach (const auto &leg, m_orders) {
			switch (leg->GetLeg()) {
				case 1:
				case 2:
					if (leg->IsActive()) {
						return;
					}
					break;
				case 3:
					if (leg->IsStarted()) {
						return;
					}
					Assert(leg->IsActive());
					break;
			}
		}

		Twd::Position &leg = GetLeg(3);
		const auto &stat = m_stat[PAIR_BC];

		if (leg.IsByRising()) {
			const auto &statData
				= stat.service->GetData(stat.bestBid.source);
			if (statData.current.theo > statData.current.emaSlow) {
				return;
			}
		} else {
			const auto &statData
				= stat.service->GetData(stat.bestAsk.source);
			if (statData.current.theo < statData.current.emaSlow) {
				return;
			}
		}

		leg.OpenAtStartPrice();
		LogAction("detected", "signal", leg.GetLeg());
		
	}

	bool CheckLeg(const Twd::Position &leg) const {

		Assert(!leg.HasActiveOrders());

		const bool isLong = leg.GetType() == Position::TYPE_LONG;
		const Security &security = leg.GetSecurity();

		const auto &printTradingRecordStart = [&](TradingRecord &record) {
			record
				% security
				% security.GetSource().GetTag()
				% leg.GetLeg()
				% (isLong ? "long" : "short");
		};

		if (!HasOpportunity()) {
			GetTradingLog().Write(
				"\tloss-detected\t%1%\t%2%\tleg: %3%\t%4%\tY1 = %5%, Y2 = %6%",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record % m_opportunity[0] % m_opportunity[1];
				});
			return false;
		}

		const auto &openPrice = security.DescalePrice(leg.GetOpenPrice());

		if (
				(isLong && openPrice < security.GetAskPrice())
				|| (!isLong && openPrice > security.GetBidPrice())) {
			GetTradingLog().Write(
				"\tloss-detected\t%1%\t%2%\tleg: %3%\t%4%\t%5% %6% %7%",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					if (isLong) {
						record % openPrice % '<' % security.GetAskPrice();
					} else {
						record % openPrice % '>' % security.GetBidPrice();
					}
				});
			return false;
		}

		return true;

	}

	void CloseLeg(Twd::Position &leg) {
		Assert(!leg.HasActiveOrders());
		if (IsZero(leg.GetCloseStartPrice())) {
			leg.CloseAtStartPrice(Position::CLOSE_TYPE_STOP_LOSS);
		} else {
			leg.CloseAtCurrentPrice(Position::CLOSE_TYPE_STOP_LOSS);
		}
	}

	void OnCancel(
			const char *reason,
			const Twd::Position &reasonOrder,
			bool showOrderPnl = false,
			bool showTotalPnl = false) {
		LogAction(
			"canceled",
			reason,
			reasonOrder.GetLeg(),
			&reasonOrder,
			showOrderPnl,
			showTotalPnl);
		m_orders.fill(boost::shared_ptr<Twd::Position>());
		CheckOpenPossibility();
	}

private:

	void WriteLogHead() {
		m_strategyLog.Write(
			[&](StrategyLogRecord &record) {
				record
					%	"No"
					%	"Time"
					%	"Action"
					%	"Action reason"
					%	"Action legs"
					%	"Action orders count"
					%	"PnL order (price)"
					%	"PnL order (vol)"
					%	"PnL total (price)"
					%	"PnL total (vol)"
					%	"Y1"
					%	"Y2";
				foreach (const auto &stat, m_stat) {
					const char *pair
						= stat
							.service
							->GetSecurity(0)
							.GetSymbol()
							.GetSymbol()
							.c_str();
					record
						%	pair % '\0' % " ECN"
						%	pair % '\0' % " direction"
						%	pair % '\0' % " leg no."
						%	pair % '\0' % " order state" 
						%	pair % '\0' % " price start"
						%	pair % '\0' % " price exec"
						%	pair % '\0' % " bid"
						%	pair % '\0' % " ask"
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
			});
	}

	void LogAction(
			const char *action,
			const char *reason,
			size_t actionLeg,
			const Twd::Position *const reasonOrder = nullptr,
			bool showOrderPnl = false,
			bool showTotalPnl = false) {
		const char *actionLegStr;
		switch (actionLeg) {
			case 1:
				actionLegStr = "1";
				break;
			case 2:
				actionLegStr = "2";
				break;
			case 3:
				actionLegStr = "3";
				break;
			default:
				actionLegStr = "unknown";
				AssertEq(1, actionLeg);
				break;
		}
		LogAction(
			action,
			reason,
			actionLegStr,
			reasonOrder,
			showOrderPnl,
			showTotalPnl);
	}

	void LogAction(
			const char *action,
			const char *reason,
			const char *actionLegs,
			const Twd::Position *const reasonOrder = nullptr,
			bool showOrderPnl = false,
			bool showTotalPnl = false) {

		Assert(IsActive());

		const auto &writePair = [&](size_t pair, StrategyLogRecord &record) {
			const auto &stat = m_stat[pair];
			const auto &order = *m_orders[pair];
			const Security &security = order.GetSecurity();
			record
				%	security.GetSource().GetTag()
				%	(order.GetType() == Position::TYPE_LONG ? "buy" : "sell")
				%	order.GetLeg();
			if (!order.IsStarted()) {
				AssertEq(0, order.GetOrdersCount());
				record % "wait";
			} else if (order.IsClosed()) {
				record % "closed";
			} else if (!IsZero(order.GetCloseStartPrice())) {
				record % "closing";
			} else if (order.IsOpened()) {
				record % "opened";
			} else if (!order.HasActiveOrders()) {
				record % "canceled";
			} else {
				record % "opening";
			}
			if (!IsZero(order.GetCloseStartPrice())) {
				record % security.DescalePrice(order.GetCloseStartPrice());
				if (order.IsClosed()) {
					record % security.DescalePrice(order.GetClosePrice());
				} else {
					record % ' ';
				}
			} else {
				record % security.DescalePrice(order.GetOpenStartPrice());
				if (order.IsOpened()) {
					record % security.DescalePrice(order.GetOpenPrice());
				} else {
					record % ' ';
				}
			}
			record 
				%	security.GetBidPrice()
				%	security.GetAskPrice();
			const auto &data = stat.service->GetData(
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

		const auto &writePnlOrder = [](
				const Twd::Position &order,
				StrategyLogRecord &record) {
			Assert(order.IsOpened());
			const auto &pnlScaled = order.GetType() == Position::TYPE_LONG
				?	order.IsClosed()
						?	order.GetClosePrice() - order.GetOpenPrice()
						:	order.GetOpenStartPrice() - order.GetOpenPrice()  
				:	order.IsClosed()
						?	order.GetOpenPrice() - order.GetClosePrice() 
						:	order.GetOpenPrice() - order.GetOpenStartPrice();
			const double pnl = order.GetSecurity().DescalePrice(pnlScaled);
			record % pnl % (pnl * order.GetOpenedQty());
		};
		
		const auto &writePnlOrderIfSet = [&](StrategyLogRecord &record) {
			if (!showOrderPnl || !reasonOrder) {
				Assert(!showOrderPnl);
				record % ' ' % ' ';
				return;
			}
			writePnlOrder(*reasonOrder, record);
		};

		const auto &writePnlTotal = [&](StrategyLogRecord &record) {
			if (!showTotalPnl) {
				record % ' ' % ' ';
				return;
			}
			if (GetLeg(3).IsActive()) {
				Assert(GetLeg(1).IsOpened());
				Assert(GetLeg(1).IsClosed());
				Assert(!GetLeg(2).IsOpened());
				writePnlOrder(GetLeg(1), record);
			} else {
				record % '-' % '-';
			}
		};

		m_strategyLog.Write(
			[&](StrategyLogRecord &record) {
				record
					% m_opportunityNo
					% GetContext().GetCurrentTime()
					% action
					% reason
					% actionLegs;
				if (reasonOrder) {
					record % reasonOrder->GetOrdersCount();
				} else {
					record % ' ';
				}	
				writePnlOrderIfSet(record);
				writePnlTotal(record);
				record
					%	m_opportunity[0]
					%	m_opportunity[1];
				for (size_t i = 0; i < numberOfPairs; ++i) {
					writePair(i, record);
				}
			});

	}

protected:

	virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
		//...//
	}

private:

	const size_t m_levelsCount;
	const Qty m_qty;

	std::ofstream m_strategyLogFile;
	StrategyLog m_strategyLog;

	boost::array<Stat, numberOfPairs> m_stat;

	boost::array<double, 2> m_opportunity;
	boost::array<double, 2> m_reportedOpportunity;
	const double m_opportunityReportStep;

	struct {
		size_t legsNo[2];
		bool isBuy[2];
		boost::optional<bool> isRising;
	} m_opportunitySource;

	size_t m_opportunityNo;
	Orders m_orders;

};

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
