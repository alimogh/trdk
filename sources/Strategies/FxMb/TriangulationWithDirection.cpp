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
namespace accs = boost::accumulators;

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
			Dump(os, "\t");
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

	enum Y {
		Y1,
		Y2,
		numberOfYs
	};

	struct Detection {
		
		Pair fistLeg;
		Y y;

		boost::array<PairSpeed, numberOfPairs> speed;

	};

	enum ProfitLossTest {
		PLT_LOSS = -1,
		PLT_NONE = 0,
		PLT_PROFIT
	};

public:

	explicit TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, "TriangulationWithDirection", tag),
		m_levelsCount(
			conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
		m_qty(conf.ReadTypedKey<Qty>("qty")),
		m_comission(conf.ReadTypedKey<double>("commission")),
		m_currentY(numberOfYs),
		m_opportunityReportStep(
			conf.ReadTypedKey<double>("opportunity_report_step")),
		m_opportunityNo(0) {

		GetLog().Info("Commission: %1%.", m_pnl.comission);

		{
			const Stat def = {};
			m_stat.fill(def);
		}
		m_yDetected.fill(.0);
		m_yCurrent.fill(.0);
		m_yDetectedReported.fill(.0);
		
		if (conf.ReadBoolKey("log.strategy")) {
		
			const pt::ptime &now = GetContext().GetStartTime();
			boost::format fileName(
				"s_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d.log");
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
		
			GetContext().GetLog().Info("Triangles log: %1%.", logPath);
			fs::create_directories(logPath.branch_path());
			m_strategyLogFile.open(
				logPath.string().c_str(),
				std::ios::app | std::ios::ate);
			if (!m_strategyLogFile) {
				throw ModuleError("Failed to open triangles log file");
			}
			m_strategyLog.EnableStream(m_strategyLogFile);

		}

		if (conf.ReadBoolKey("log.pnl")) {
		
			const auto &logPath
				= context.GetSettings().GetLogsDir() / "pnl.log";
		
			GetContext().GetLog().Info("PnL log: %1%.", logPath);
			
			const bool isNewFile = !fs::exists(logPath);
			if (isNewFile) {
				fs::create_directories(logPath.branch_path());
			}
			
			m_pnlLogFile.open(
				logPath.string().c_str(),
				std::ios::app | std::ios::ate);
			if (!m_pnlLogFile) {
				throw ModuleError("Failed to open PnL log file");
			}
			m_pnlLog.EnableStream(m_pnlLogFile);

			if (isNewFile) {
				WritePnlLogHead();
			}

		}

	}

	virtual ~TriangulationWithDirection() {
		//...//
	}

public:

	bool HasDetectedOpportunity() const {
		return
			(m_yDetected[Y1] >= 1.0 && m_yDetected[Y2] > .0)
			|| (m_yDetected[Y2] >= 1.0 && m_yDetected[Y1] > .0);
	}

	bool HasCurrentOpportunity() const {
		AssertNe(numberOfYs, m_currentY);
		return
			(m_yCurrent[Y1] >= 1.0
				&& m_yCurrent[Y2] > .0
				&& m_currentY == Y1)
			|| (
				m_yCurrent[Y2] >= 1.0
				&& m_yCurrent[Y1] > .0
				&& m_currentY == Y2);
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
				WriteStrategyLogHead();
			}
		}

// @todo see https://trello.com/c/ONnb5ai2
//		m_stat.push_back(statService);

	}

	virtual void OnServiceDataUpdate(
			const Service &service,
			const TimeMeasurement::Milestones &timeMeasurement) {

		UpdateStat(service);

		if (IsActive()) {
			CheckClosePossibility(timeMeasurement);
		} else {
			CheckOpenPossibility(timeMeasurement);
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
						if (!CheckProfitLoss(firstLeg, false)) {
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

			return;
		
		} else if (!IsZero(order.GetCloseStartPrice())) {
			
			AssertLt(0, order.GetOpenedQty());
			AssertEq(1, order.GetLeg());
			
			if (order.GetActiveQty() == 0) {
				OnCancel("exec report", order);
				return;
			} else if (
					order.GetCloseType() != Position::CLOSE_TYPE_TAKE_PROFIT) {
				CloseLeg(order, order.GetCloseType());
				return;
			}

			order.SetCloseStartPrice(0);
		
		}
 
 		Assert(order.IsOpened());
		Assert(IsZero(order.GetCloseStartPrice()));
		AssertEq(0, order.GetClosedQty());
		Assert(!order.HasActiveOrders());

		switch (order.GetLeg()) {
			
			case 1:
				if (CheckProfitLoss(order, true)) {
					return;
				}
				StartLeg2(order);
				LogAction(
					"executed",
					"exec report",
					"1 -> 2",
					&order);
				break;

			case 3:
				LogAction(
					"executed",
					"exec report",
					order.GetLeg(),
					&order);
				m_orders.fill(boost::shared_ptr<Twd::Position>());
#				ifdef BOOST_ENABLE_ASSERT_HANDLER
					m_currentY = numberOfYs;
#				endif
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
			m_yDetected.fill(0);
			m_yCurrent.fill(0);
		
		} else if (
				IsZero(m_stat[PAIR_AB].bestBid.price)
				||	IsZero(m_stat[PAIR_AB].bestAsk.price)
				||	IsZero(m_stat[PAIR_BC].bestBid.price)
				||	IsZero(m_stat[PAIR_BC].bestAsk.price)
				||	IsZero(m_stat[PAIR_AC].bestBid.price)
				||	IsZero(m_stat[PAIR_AC].bestAsk.price)) {
		
			m_yDetected.fill(0);
			m_yCurrent.fill(0);

		} else {

			m_yDetected[Y1]
				= m_stat[PAIR_AB].bestBid.price
					* m_stat[PAIR_BC].bestBid.price
					* (1.0 / m_stat[PAIR_AC].bestAsk.price);
			m_yDetected[Y2]
				= m_stat[PAIR_AC].bestBid.price
					* (1.0 / m_stat[PAIR_BC].bestAsk.price)
					* (1.0 / m_stat[PAIR_AB].bestAsk.price);
			
			AssertGt(1.1, m_yDetected[Y1]);
			AssertLt(.9, m_yDetected[Y1]);
			
			AssertGt(1.1, m_yDetected[Y2]);
			AssertLt(.9, m_yDetected[Y2]);

			if (IsActive()) {
			
				m_yCurrent[Y1]
					= m_orders[PAIR_AB]->GetSecurity().GetBidPrice()
						* m_orders[PAIR_BC]->GetSecurity().GetBidPrice()
						* (1 / m_orders[PAIR_AC]->GetSecurity().GetAskPrice());
				m_yCurrent[Y2]
					= m_orders[PAIR_AC]->GetSecurity().GetBidPrice()
						* (1 / m_orders[PAIR_BC]->GetSecurity().GetAskPrice())
						* (1 / m_orders[PAIR_AB]->GetSecurity().GetAskPrice());

				AssertGt(1.1, m_yCurrent[Y1]);
				AssertLt(.9, m_yCurrent[Y1]);
			
				AssertGt(1.1, m_yCurrent[Y2]);
				AssertLt(.9, m_yCurrent[Y2]);
			
			}

			AssertGt(1.1, m_yDetected[Y1]);
			AssertLt(.9, m_yDetected[Y1]);
			
			AssertGt(1.1, m_yDetected[Y2]);
			AssertLt(.9, m_yDetected[Y2]);

		}

		const auto &isTimeToReport = [this](double current, double reported) {
			return
				reported > 1.0 != current > 1.0
				|| current < (reported - m_opportunityReportStep)
				|| (reported + m_opportunityReportStep) < current;
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

	double GetCurrentYTargeted() const {
		Assert(IsActive());
		Assert(
			m_orders[PAIR_AB]->IsOpened()
				&& m_orders[PAIR_BC]->IsOpened()
				&& m_orders[PAIR_AC]->IsOpened());
		Assert(
			!m_orders[PAIR_AB]->IsClosed()
				&& !m_orders[PAIR_BC]->IsClosed()
				&& !m_orders[PAIR_AC]->IsClosed());
		AssertLt(0, m_orders[PAIR_AB]->GetOpenStartPrice());
		AssertLt(0, m_orders[PAIR_BC]->GetOpenStartPrice());
		AssertLt(0, m_orders[PAIR_AC]->GetOpenStartPrice());
		if (m_orders[PAIR_AB]->GetType() == Position::TYPE_SHORT) {
			const auto &acAsk
				= m_orders[PAIR_AC]->GetSecurity().DescalePrice(
					m_orders[PAIR_AC]->GetOpenStartPrice());
			return
				m_orders[PAIR_AB]->GetSecurity().DescalePrice(
						m_orders[PAIR_AB]->GetOpenStartPrice())
					* m_orders[PAIR_BC]->GetSecurity().DescalePrice(
						m_orders[PAIR_BC]->GetOpenStartPrice())
					* (1.0 / acAsk);
		} else {
			AssertEq(Position::TYPE_LONG, m_orders[PAIR_AB]->GetType());
			const auto &bcAsk = m_orders[PAIR_BC]->GetSecurity().DescalePrice(
				m_orders[PAIR_BC]->GetOpenStartPrice());
			const auto abAsk = m_orders[PAIR_AB]->GetSecurity().DescalePrice(
				m_orders[PAIR_AB]->GetOpenStartPrice());
			return
				m_orders[PAIR_AC]->GetSecurity().DescalePrice(
						m_orders[PAIR_AC]->GetOpenStartPrice())
					* (1.0 / bcAsk)
					* (1.0 / abAsk);
		}
	}

	double GetCurrentYExecuted() const {
		Assert(IsActive());
		Assert(
			m_orders[PAIR_AB]->IsOpened()
				&& m_orders[PAIR_BC]->IsOpened()
				&& m_orders[PAIR_AC]->IsOpened());
		Assert(
			!m_orders[PAIR_AB]->IsClosed()
				&& !m_orders[PAIR_BC]->IsClosed()
				&& !m_orders[PAIR_AC]->IsClosed());
		AssertLt(0, m_orders[PAIR_AB]->GetOpenStartPrice());
		AssertLt(0, m_orders[PAIR_BC]->GetOpenStartPrice());
		AssertLt(0, m_orders[PAIR_AC]->GetOpenStartPrice());
		if (m_orders[PAIR_AB]->GetType() == Position::TYPE_SHORT) {
			const auto &acAsk
				= m_orders[PAIR_AC]->GetSecurity().DescalePrice(
					m_orders[PAIR_AC]->GetOpenPrice());
			return
				m_orders[PAIR_AB]->GetSecurity().DescalePrice(
						m_orders[PAIR_AB]->GetOpenPrice())
					* m_orders[PAIR_BC]->GetSecurity().DescalePrice(
						m_orders[PAIR_BC]->GetOpenPrice())
					* (1.0 / acAsk);
		} else {
			AssertEq(Position::TYPE_LONG, m_orders[PAIR_AB]->GetType());
			const auto &bcAsk = m_orders[PAIR_BC]->GetSecurity().DescalePrice(
				m_orders[PAIR_BC]->GetOpenPrice());
			const auto abAsk = m_orders[PAIR_AB]->GetSecurity().DescalePrice(
				m_orders[PAIR_AB]->GetOpenPrice());
			return
				m_orders[PAIR_AC]->GetSecurity().DescalePrice(
						m_orders[PAIR_AC]->GetOpenPrice())
					* (1.0 / bcAsk)
					* (1.0 / abAsk);
		}
	}

	bool Detect(Detection &result) const {

		Assert(HasDetectedOpportunity());
		Assert(!IsActive());
		AssertEq(numberOfYs, m_currentY);

		for (size_t pair = 0; pair < result.speed.size(); ++pair) {
			
			PairSpeed &speed = result.speed[pair];
			const auto &stat = m_stat[pair];

			{
				const auto &data = stat.service->GetData(stat.bestBid.source);
				const bool isRising
					= data.current.theo > data.current.emaFast
						&& data.current.emaFast > data.current.emaSlow;
				speed.rising = isRising
					?	data.current.theo - data.prev2.theo
					:	std::numeric_limits<double>::quiet_NaN();
			}

			{
				const auto &data = stat.service->GetData(stat.bestAsk.source);
				const bool isFalling
					= data.current.theo < data.current.emaFast
						&& data.current.emaFast < data.current.emaSlow;
				speed.falling = isFalling
					?	data.prev2.theo - data.current.theo
					:	std::numeric_limits<double>::quiet_NaN();
			}

		}

		struct SpeedTest {
		
			Pair fastestPair;
			double fastestSpeed;

			SpeedTest()
				: fastestPair(numberOfPairs),
				fastestSpeed(std::numeric_limits<double>::quiet_NaN()) {
			}

			void Test(const Pair &pair, double speed) {
				if (
						isnan(speed)
						|| (!isnan(fastestSpeed) && fastestSpeed >= speed)) {
					return;
				}
//! @todo:		compare prices for pairs https://trello.com/c/NaVQzajm
				fastestSpeed = speed;
				fastestPair = pair;
			}

		};

		if (m_yDetected[Y1] >= 1.0) {
			
			SpeedTest speedTest;
			speedTest.Test(PAIR_AB, result.speed[PAIR_AB].falling);
			speedTest.Test(PAIR_BC, result.speed[PAIR_BC].falling);
			speedTest.Test(PAIR_AC, result.speed[PAIR_AC].rising);

			if (speedTest.fastestPair != numberOfPairs) {
			
				Assert(!isnan(speedTest.fastestSpeed));
			
				result.y = Y1;
			
				if (!isnan(result.speed[PAIR_AB].falling)) {
//! @todo:			Assert(isnan(result.speed[PAIR_AC].rising)); https://trello.com/c/NaVQzajm
					result.fistLeg = PAIR_AB;
					return true;
				} else if (!isnan(result.speed[PAIR_AC].rising)) {
					result.fistLeg = PAIR_AC;
					return true;
				}
			
			}

		}

		if (m_yDetected[Y2] >= 1.0) {

			SpeedTest speedTest;
			speedTest.Test(PAIR_AB, result.speed[PAIR_AB].rising);
			speedTest.Test(PAIR_BC, result.speed[PAIR_BC].rising);
			speedTest.Test(PAIR_AC, result.speed[PAIR_AC].falling);

			if (speedTest.fastestPair != numberOfPairs) {
			
				Assert(!isnan(speedTest.fastestSpeed));
			
				result.y = Y2;
			
				if (!isnan(result.speed[PAIR_AB].rising)) {
//! @todo:			Assert(isnan(result.speed[PAIR_AC].falling));  https://trello.com/c/NaVQzajm
					result.fistLeg = PAIR_AB;
					return true;
				} else if (!isnan(result.speed[PAIR_AC].falling)) {
					result.fistLeg = PAIR_AC;
					return true;
				}
			
			}

		}

		return false;

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

	void CheckOpenPossibility(
			const TimeMeasurement::Milestones &timeMeasurement) {

		Assert(!IsActive());

 		if (!HasDetectedOpportunity()) {
			timeMeasurement.Measure(
				TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
 			return;
 		}

		timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START);

		Detection detection;
		if (!Detect(detection)) {
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
			}
		};

		const auto &newPosition = [&](
				const Pair &pair,
				size_t legNo,
				bool isBuy,
				bool isBaseCurrency) 
				-> boost::shared_ptr<Twd::Position> {
			const auto &stat = m_stat[pair];
			const auto &ecn = !isBuy
				?	stat.bestBid.source
				:	stat.bestAsk.source;
			const Security &security = stat.service->GetSecurity(ecn);
			const auto &currency = !isBaseCurrency
				?	security.GetSymbol().GetCashQuoteCurrency()
				:	security.GetSymbol().GetCashBaseCurrency();
			boost::shared_ptr<Twd::Position> result;
			if (isBuy) {
				if (security.GetAskQty() == 0) {
					throw HasNotMuchOpportunity(security, "ask");
				}
				Assert(!IsZero(security.GetAskPrice()));
				result.reset(
					new Twd::LongPosition(
						*this,
						GetContext().GetTradeSystem(ecn),
						//! @todo fixme:
						const_cast<Security &>(security),
						currency,
						0,
						legNo == 1
							?	security.GetAskPriceScaled()
							:	0,
						timeMeasurement,
						pair,
						legNo,
						0));
			} else {
				if (security.GetBidQty() == 0) {
					throw HasNotMuchOpportunity(security, "bid");
				}
				Assert(!IsZero(security.GetBidPrice()));
				result.reset(
					new Twd::ShortPosition(
						*this,
						GetContext().GetTradeSystem(ecn),
						//! @todo fixme:
						const_cast<Security &>(security),
						currency,
						0,
						legNo == 1
							?	security.GetBidPriceScaled()
							:	0,
						timeMeasurement,
						pair,
						legNo,
						0));
			}
			return std::move(result);
		};

		try {

			AssertNe(PAIR_BC, detection.fistLeg);

			Orders orders = {
				newPosition(
					PAIR_AB,
					detection.fistLeg == PAIR_AB ? 1 : 2,
					detection.y == Y2,
					detection.fistLeg == PAIR_AB), 
				newPosition(
					PAIR_BC,
					3,
					detection.y == Y2,
					detection.fistLeg == PAIR_AB),
				newPosition(
					PAIR_AC,
					detection.fistLeg == PAIR_AC ? 1 : 2,
					detection.y == Y1,
					detection.fistLeg == PAIR_AC)
			};

			{
				Twd::Position &firstLeg = GetLeg(orders, 1);
				firstLeg.SetPlanedQty(m_qty);
				timeMeasurement.Measure(
					TimeMeasurement::SM_STRATEGY_EXECUTION_START);
				firstLeg.OpenAtStartPrice();
			}

			orders.swap(m_orders);
			++m_opportunityNo;
			m_currentY = detection.y;
			LogAction("detected", "signal", "1", nullptr, &detection.speed);

			timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);

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

	void CheckClosePossibility(
			const TimeMeasurement::Milestones &timeMeasurement) {

		foreach (const auto &leg, m_orders) {
			switch (leg->GetLeg()) {
				case 1:
				case 2:
					if (leg->IsActive()) {
						timeMeasurement.Measure(
							TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
						return;
					}
					break;
				case 3:
					if (leg->IsStarted()) {
						timeMeasurement.Measure(
							TimeMeasurement::SM_STRATEGY_WITHOUT_DECISION);
						return;
					}
					Assert(leg->IsActive());
					break;
			}
		}

		timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_START);

		Twd::Position &leg = GetLeg(3);
		AssertEq(0, leg.GetOpenStartPrice());
		AssertLt(0, leg.GetPlanedQty());

		const auto &stat = m_stat[PAIR_BC];
		const auto &ecn = leg.GetSecurity().GetSource().GetIndex();

		if (GetLeg(1).GetType() == Position::TYPE_LONG) {
			const auto &statData = stat.service->GetData(ecn);
			if (statData.current.theo > statData.current.emaSlow) {
				return;
			}
		} else {
			const auto &statData = stat.service->GetData(ecn);
			if (statData.current.theo < statData.current.emaSlow) {
				return;
			}
		}

		leg.UpdateStartOpenPriceFromCurrent();
		
		timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_EXECUTION_START);
		leg.OpenAtStartPrice();
		LogAction("detected", "signal", leg.GetLeg());

		timeMeasurement.Measure(TimeMeasurement::SM_STRATEGY_DECISION_STOP);
		
	}

	bool CheckProfitLoss(Twd::Position &firstLeg, bool isJustOpened) {
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
			LogAction("executed", "exec report", firstLeg.GetLeg(), &firstLeg);
		}
		CloseLeg(firstLeg, closeType);
		LogAction("canceling", closeTypeStr, firstLeg.GetLeg());
		return true;
	}

	ProfitLossTest CheckLeg(const Twd::Position &leg) const {

		Assert(!leg.HasActiveOrders());

		const bool isLong = leg.GetType() == Position::TYPE_LONG;
		const Security &security = leg.GetSecurity();

		const auto &printTradingRecordStart = [&](TradingRecord &record) {
			record
				% security
				% security.GetSource().GetTag()
				% m_opportunityNo
				% (isLong ? "long" : "short");
		};

		if (!HasCurrentOpportunity()) {
			GetTradingLog().Write(
				"\tloss-detected\t%1%\t%2%\topp.: %3%\t%4%"
					"\tY1 = %5%, Y2 = %6%, current: Y%7%",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record % m_yCurrent[Y1] % m_yCurrent[Y2] % (m_currentY + 1);
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
			GetTradingLog().Write(
				"\tprofit-detected\t%1%\t%2%\topp.: %3%\t%4%"
					"\t%5% -> %6%  = %7$.7f",
				[&](TradingRecord &record) {
					printTradingRecordStart(record);
					record % openPrice % currentPrice % seenProfit;
				});
			return PLT_PROFIT;
		}

		return PLT_NONE;

	}

	void CloseLeg(Twd::Position &leg, const Position::CloseType &closeType) {
		Assert(!leg.HasActiveOrders());
		if (IsZero(leg.GetCloseStartPrice())) {
			leg.CloseAtStartPrice(closeType);
		} else {
			leg.CloseAtCurrentPrice(closeType);
		}
	}

	void StartLeg2(const Twd::Position &leg1) {
		
		Twd::Position &leg2 = GetLeg(2);
		Twd::Position &leg3 = GetLeg(3);

		AssertEq(0, leg2.GetOpenStartPrice());
		AssertEq(0, leg3.GetOpenStartPrice());

		AssertEq(0, leg2.GetPlanedQty());
		AssertEq(0, leg3.GetPlanedQty());
		
		const auto &leg1Security = leg1.GetSecurity();
		const double leg1Price = leg1Security.DescalePrice(leg1.GetOpenPrice());
		const auto leg1Vol = leg1Price * leg1.GetOpenedQty();

		const Security &leg3Security = leg3.GetSecurity();
		const auto leg3Price = 120.0;
		const auto &leg3QuoteCurrency
			= leg3Security.GetSymbol().GetCashQuoteCurrency();
		const auto &leg1QuoteCurrency
			= leg1Security.GetSymbol().GetCashQuoteCurrency();
		const auto leg2Qty = leg3QuoteCurrency == leg1QuoteCurrency
			?	leg1Vol / leg3Price
			:	leg1Vol * leg3Price;

		leg2.UpdateStartOpenPriceFromCurrent();
		//! @todo remove "to qty"
		leg2.SetPlanedQty(Qty(leg2Qty));

		//! @todo remove "to qty"
		leg3.SetPlanedQty(Qty(leg1Vol));

		leg2.OpenAtStartPrice();

	}

	void OnCancel(const char *reason, const Twd::Position &reasonOrder) {
		LogAction("canceled", reason, reasonOrder.GetLeg(), &reasonOrder);
		m_orders.fill(boost::shared_ptr<Twd::Position>());
#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			m_currentY = numberOfYs;
#		endif
		CheckOpenPossibility(TimeMeasurement::Milestones());
	}

private:

	void WriteStrategyLogHead() {
		m_strategyLog.Write(
			[&](StrategyLogRecord &record) {
				record
					%	"No"
					%	"Time"
					%	"Action"
					%	"Action reason"
					%	"Action legs"
					%	"Action orders count"
					%	"Y1 detected"
					%	"Y2 detected"
					%	"New Y1"
					%	"New Y2"
					%	"Y executed"
					%	"Y targeted";
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
						%	pair % '\0' % " leg no."
						%	pair % '\0' % " direction"
						%	pair % '\0'	% " order qty"
						%	pair % '\0'	% " order currency"
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
				record % "Start: " % '\0' % GetContext().GetStartTime();
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

	void WritePnlLogHead() {
		m_pnlLog.Write(
			[&](StrategyLogRecord &record) {
				record
					%	"Date and logs #"
					%	"Triangle ID (winner)"
					%	"Winners"
					%	"Triangle ID (loser)"
					%	"Losers"
					%	"Avg winners"
					%	"Avg losers"
					%	"# of winners"
					%	"# of losers"
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
	};

	void LogAction(
			const char *action,
			const char *reason,
			size_t actionLeg,
			const Twd::Position *const reasonOrder = nullptr) {
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
			reasonOrder);
	}

	void LogAction(
			const char *action,
			const char *reason,
			const char *actionLegs,
			const Twd::Position *const reasonOrder = nullptr,
			const boost::array<PairSpeed, numberOfPairs> *speed = nullptr) {

		Assert(IsActive());

		const auto &writePair = [&](size_t pair, StrategyLogRecord &record) {
			const auto &stat = m_stat[pair];
			const auto &order = *m_orders[pair];
			const Security &security = order.GetSecurity();
			record
				% security.GetSource().GetTag()
				% order.GetLeg()
				% (order.GetType() == Position::TYPE_LONG ? "buy" : "sell");
			if (order.GetPlanedQty() == 0) {
				record % ' ';
			} else {
				record % order.GetPlanedQty();
			}
			record % ConvertToIsoPch(order.GetCurrency());
			if (!order.IsStarted()) {
				AssertEq(0, order.GetOrdersCount());
				AssertEq(0, order.GetOpenStartPrice());
				record % "wait";
			} else if (order.IsClosed()) {
				AssertLt(0, order.GetOpenStartPrice());
				record % "closed";
			} else if (!IsZero(order.GetCloseStartPrice())) {
				AssertLt(0, order.GetOpenStartPrice());
				record % "closing";
			} else if (order.IsOpened()) {
				AssertLt(0, order.GetOpenStartPrice());
				record % "opened";
			} else if (!order.HasActiveOrders()) {
				record % "canceled";
			} else {
				AssertLt(0, order.GetOpenStartPrice());
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
				const auto &openStartPrice = order.GetOpenStartPrice();
				if (IsZero(openStartPrice)) {
					record % ' ';
				} else {
					record % security.DescalePrice(openStartPrice);
				}
				if (order.IsOpened()) {
					record % security.DescalePrice(order.GetOpenPrice());
				} else {
					record % ' ';
				}
			}
			// Chosen ECN bid/ask:  ///////////////////////////////////////////////////////
			record 
				%	security.GetBidPrice()
				%	security.GetAskPrice();
			// Best bid/ask and ECNs: ////////////////////////////////////////////////////
			const Context &context = GetContext();
			record
				%	stat.bestBid.price
				%	context.GetMarketDataSource(stat.bestBid.source).GetTag()
				%	stat.bestAsk.price
				%	context.GetMarketDataSource(stat.bestAsk.source).GetTag();
			// Rising/falling speed: ///////////////////////////////////////////////////////
			if (speed) {
				if (!isnan((*speed)[pair].rising)) {
					record % (*speed)[pair].rising;
				} else {
					record % ' ';
				}
				if (!isnan((*speed)[pair].falling)) {
					record % (*speed)[pair].falling;
				} else {
					record % ' ';
				}
			} else {
				record % ' ' % ' ';
			}
			// Stat data: //////////////////////////////////////////////////////////////////
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
			////////////////////////////////////////////////////////////////////////////////
		};

		const bool isTriangleCanceled
			= reasonOrder
				&& reasonOrder->GetLeg() == 1
				&& reasonOrder->IsClosed();
		const bool isTriangleCompleted
			= reasonOrder
				&& reasonOrder->GetLeg() == 3
				&& reasonOrder->IsOpened();
		Assert(
			isTriangleCanceled != isTriangleCompleted
				|| !isTriangleCompleted);

		const double yExecuted = isTriangleCompleted
			?	GetCurrentYExecuted()
			:	.0;

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
				record % m_yDetected[Y1] % m_yDetected[Y2];
				record % m_yCurrent[Y1] % m_yCurrent[Y2];
				if (isTriangleCompleted) {
					record % yExecuted % GetCurrentYTargeted();
				} else {
					record % ' ' % ' ';
				}
				for (size_t i = 0; i < numberOfPairs; ++i) {
					writePair(i, record);
				}
			});


		const auto &writePnl = [&](StrategyLogRecord &record) {
			const auto pnl = m_pnl.winnersPnl + m_pnl.losersPnl;
			record % (pnl - m_pnl.comission) % pnl % m_pnl.comission;
		};
		
		if (isTriangleCompleted) {
			
			m_pnlLog.Write(
				[&](StrategyLogRecord &record) {

					m_pnl.comission += m_comission * 3;

					const bool isWinner = yExecuted >= 1;
					if (isWinner) {
						m_pnl.winners(yExecuted);
						m_pnl.winnersPnl += yExecuted - 1;
					} else {
						m_pnl.losers(yExecuted);
						m_pnl.losersPnl += yExecuted - 1;
					}

					record % ' ';

					if (!isWinner) {
						record % ' ' % ' ';
					}
					record % m_opportunityNo % yExecuted;

					if (isWinner) {
						record % ' ' % ' ';
						record % accs::mean(m_pnl.winners);
						record % ' ';
						record % accs::count(m_pnl.winners);
						record % ' ';
					} else {
						record % ' ' % accs::mean(m_pnl.losers);
						record % ' ' % accs::count(m_pnl.losers);
					}

					for (auto i = 0; i < 8; ++i) {
						record % ' ';
					}

					writePnl(record);

				});

		} else if (isTriangleCanceled) {

			const Security &security = reasonOrder->GetSecurity();
			const auto &entryPrice
				= security.DescalePrice(reasonOrder->GetOpenPrice());
			const auto &exitPrice
				= security.DescalePrice(reasonOrder->GetClosePrice());
			const auto yExecuted = reasonOrder->GetType() == Position::TYPE_LONG
				?	(1 / entryPrice) * exitPrice
				:	entryPrice * (1 / exitPrice);

			m_pnlLog.Write(
				[&](StrategyLogRecord &record) {

					m_pnl.comission += m_comission * 2;
			
					const bool isWinner = yExecuted >= 1;
					if (isWinner) {
						m_pnl.winnersCancels(yExecuted);
						m_pnl.winnersPnl += yExecuted - 1;
					} else {
						m_pnl.losersCancels(yExecuted);
						m_pnl.losersPnl += yExecuted - 1;
					}

					{
						const auto skipCount = isWinner ? 9 : 9 + 2;
						for (auto i = 0; i < skipCount; ++i) {
							record % ' ';
						}
					}

					record % m_opportunityNo % yExecuted;

					if (isWinner) {
						record % ' ' % ' ';
						record % accs::mean(m_pnl.winnersCancels);
						record % ' ';
						record % accs::count(m_pnl.winnersCancels);
						record % ' ';
					} else {
						record % ' ' % accs::mean(m_pnl.losersCancels);
						record % ' ' % accs::count(m_pnl.losersCancels);
					}

					writePnl(record);

				});

		}

	}

protected:

	virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
		//...//
	}

private:

	const size_t m_levelsCount;
	const Qty m_qty;
	const double m_comission;

	std::ofstream m_strategyLogFile;
	StrategyLog m_strategyLog;

	std::ofstream m_pnlLogFile;
	StrategyLog m_pnlLog;
	struct Pnl {
	
		double comission;
	
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			winners;
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			winnersCancels;
		double winnersPnl;
	
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			losers;
		accs::accumulator_set<
				double,
				accs::stats<accs::tag::count, accs::tag::mean>>
			losersCancels;
	
		double losersPnl;
	
		explicit Pnl()
			: comission(.0),
			winnersPnl(.0),
			losersPnl(.0) {
		}
	
	} m_pnl;

	boost::array<Stat, numberOfPairs> m_stat;

	boost::array<double, numberOfYs> m_yDetected;
	boost::array<double, numberOfYs> m_yCurrent;
	Y m_currentY;
	boost::array<double, numberOfYs> m_yDetectedReported;
	const double m_opportunityReportStep;

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
