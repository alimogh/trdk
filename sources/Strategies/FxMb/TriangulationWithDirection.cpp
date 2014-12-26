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

	enum Pair {
		//! Like a EUR/USD.
		PAIR_AB,
		//! Like a USD/JPY.
		PAIR_BC,
		//! Like a EUR/JPY.
		PAIR_AC,
		numberOfPairs = 3
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
		m_opportunityNo(0) {

		{
			const Stat def = {};
			m_stat.fill(def);
		}
		m_opportunity.fill(.0);
		
		if (conf.ReadBoolKey("log")) {
		
			const pt::ptime &now = pt::microsec_clock::local_time();
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

	void CancelAllAndBlock(CancelAndBlockCondition &) {
		//...//
	}

public:

	void WaitForCancelAndBlock(CancelAndBlockCondition &) {
		//...//
	}

	bool HasOpportunity() const {
		return m_opportunity[0] > 1.0 || m_opportunity[1] > 1.0;
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

		if (position.HasActiveOpenOrders()) {
			return;
		}

		Twd::Position &order = dynamic_cast<Twd::Position &>(position);

		if (order.IsCompleted()) {
			//! @todo see https://trello.com/c/QOBSd8RZ
			return;
		}

		if (order.GetOpenedQty() == 0) {

			if (order.GetLeg() == 1) {
				LogAction("canceled");
				m_orders.fill(boost::shared_ptr<Twd::Position>());
				CheckOpenPossibility();
			} else {
				ReplaceOrder(order);
			}

		} else {
 
 			Assert(order.IsOpened());
			order.Complete();

			 if (order.GetLeg() == 1) {
				GetLeg(2).Open();
			 }

			LogAction("executed");
			
			if (order.GetLeg() == 3) {
				m_orders.fill(boost::shared_ptr<Twd::Position>());
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
		for (size_t ecn = 0; ecn < ecnsCount; ++ecn) {
			const Security &security = stat->service->GetSecurity(ecn);
			{
				const auto &bid = security.GetBidPrice();
				if (stat->bestBid.price < bid) {
					stat->bestBid.price = bid;
					stat->bestBid.source = ecn;
				}
			}
			{
				const auto &ask = security.GetAskPrice();
				if (stat->bestAsk.price > ask || IsZero(stat->bestAsk.price)) {
					stat->bestAsk.price = ask;
					stat->bestAsk.source = ecn;
				}
			}
		}

		if (!IsZero(stat->bestAsk.price)) {
			stat->bestAsk.price = 1.0 / stat->bestAsk.price;
		}

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

	void ReplaceOrder(Twd::Position &oldOrder) {

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
					oldOrder.GetLeg(),
					oldOrder.IsByRising()));
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
					oldOrder.GetLeg(),
					oldOrder.IsByRising()));
		}

		foreach (auto &order, m_orders) {
			if (order->GetLeg() == newOrder->GetLeg()) {
				order = newOrder;
				order->Open();
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
		
		size_t legsNo[2];
		bool isBuy[2];
		bool isRising;
		if (!IsZero(movementSpeed[0])) {
			Assert(IsZero(movementSpeed[1]));
			isRising = movementSpeed[0] > 0;
			legsNo[0] = 1;
			isBuy[0] = isRising;
			legsNo[1] = 2;
			isBuy[1] = !isRising;
		} else {
			Assert(!IsZero(movementSpeed[1]));
			isRising = movementSpeed[1] > 0;
			legsNo[0] = 2;
			isBuy[0] = !isRising;
			legsNo[1] = 1;
			isBuy[1] = isRising;
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
				const size_t pair,
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
						security.GetSymbol().GetCashCurrency(),
						m_qty,
						security.GetAskPriceScaled(),
						TimeMeasurement::Milestones(),
						legNo,
						isRising));
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
						security.GetSymbol().GetCashCurrency(),
						m_qty,
						security.GetBidPriceScaled(),
						TimeMeasurement::Milestones(),
						legNo,
						isRising));
			}
			return std::move(result);
		};

		try {

			Orders orders = {
				newPosition(PAIR_AB, legsNo[0], isRising, isBuy[0]),
				newPosition(PAIR_BC, 3, isRising, isBuy[0]),
				newPosition(PAIR_AC, legsNo[1], !isRising, isBuy[1])
			};
			GetLeg(orders, 1).Open();
			orders.swap(m_orders);

			++m_opportunityNo;
			LogAction("detected");

		} catch (const HasNotMuchOpportunity &ex) {
			GetTradingLog().Write(
				"Skipped decision:"
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

		Twd::Position *lastLeg = nullptr;
		foreach (const auto &leg, m_orders) {
			switch (leg->GetLeg()) {
				case 2:
					if (!leg->IsCompleted()) {
						return;
					}
					break;
				case 3:
					if (leg->IsStarted()) {
						return;
					}
					Assert(!leg->IsCompleted());
					lastLeg = &*leg;
					break;
			}
		}
		Assert(lastLeg);

		const auto &stat = m_stat[PAIR_BC];
		if (lastLeg->IsByRising()) {
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

		GetLeg(3).Open();
		LogAction("detected");
		
	}

private:

	void WriteLogHead() {
		m_strategyLog.Write(
			[&](StrategyLogRecord &record) {
				record
					%	"No"
					%	"Time"
					%	"Action"
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
						%	pair % '\0' % " Direction"
						%	pair % '\0' % " Leg No."
						%	pair % '\0' % " Order State"
						%	pair % '\0' % " Exec price"
						%	pair % '\0' % " Bid"
						%	pair % '\0' % " Ask"
						%	pair % '\0' % " VWAP"
						%	pair % '\0' % " VWAP Prev1"
						%	pair % '\0' % " VWAP Prev2"
						%	pair % '\0' % " fastEMA"
						%	pair % '\0' % " fastEMA Prev1"
						%	pair % '\0' % " fastEMA Prev2"
						%	pair % '\0' % " slowEMA"
						%	pair % '\0' % " slowEMA Prev1"
						%	pair % '\0' % " slowEMA Prev2";
				}
			});
	}

	void LogAction(const char *action) {

		Assert(IsActive());

		const auto &writePair = [&](size_t pair, StrategyLogRecord &record) {
			const auto &stat = m_stat[pair];
			const auto &order = *m_orders[pair];
			const auto &ecn = order.IsByRising()
				?	stat.bestBid.source
				:	stat.bestAsk.source;
			const Security &security = stat.service->GetSecurity(ecn);
			record
				%	security.GetSource().GetTag()
				%	(order.GetType() == Position::TYPE_LONG ? "buy" : "sell")
				%	order.GetLeg();
			if (!order.IsStarted()) {
				record % "wait" % '-';
			} else if (!order.IsCompleted()) {
				record
					% "active"
					% order.GetSecurity().DescalePrice(order.GetOpenStartPrice());
			} else {
				record
					% "done"
					% order.GetSecurity().DescalePrice(order.GetOpenPrice());
			}
			record 
				%	security.GetBidPrice()
				%	security.GetAskPrice();
			const auto &data = stat.service->GetData(ecn);
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

		m_strategyLog.Write(
			[&](StrategyLogRecord &record) {
				record
					%	m_opportunityNo
					%	GetContext().GetCurrentTime()
					%	action
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
