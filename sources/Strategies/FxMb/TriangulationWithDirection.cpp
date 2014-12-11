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
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

namespace trdk { namespace Strategies { namespace FxMb {
	class TriangulationWithDirection;
} } }

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

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

class Strategies::FxMb::TriangulationWithDirection : public Strategy {
		
public:
		
	typedef Strategy Base;

private:

	typedef std::vector<ScaledPrice> BookSide;

	struct Stat {

		const TriangulationWithDirectionStatService *service;
		
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

public:

	explicit TriangulationWithDirection(
			Context &context,
			const std::string &tag,
			const IniSectionRef &conf)
		: Base(context, "TriangulationWithDirection", tag),
		m_levelsCount(
			conf.GetBase().ReadTypedKey<size_t>("Common", "levels_count")),
		m_qty(conf.ReadTypedKey<double>("qty")),
		m_isLogInited(false)  {
		
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
		
	virtual void OnServiceStart(const Service &service) {
		const Stat statService = {
			boost::polymorphic_downcast<const TriangulationWithDirectionStatService *>(
				&service)
		};
		size_t index = statService.service->GetSecurity(0).GetSymbol().GetSymbol() == "EUR/USD"
			?	0
			:	statService.service->GetSecurity(0).GetSymbol().GetSymbol() == "EUR/JPY"
				?	2
				:	1;
		m_stat.resize(3);
		m_stat[index] = statService;
		// see https://trello.com/c/ONnb5ai2
//		m_stat.push_back(statService);
	}

	virtual void OnServiceDataUpdate(const Service &service) {

		////////////////////////////////////////////////////////////////////////////////
		// Updating min/max:

		UpdateStat(service);

		////////////////////////////////////////////////////////////////////////////////
		// Y1 and Y2:
		// 
		
		const double opportunite[2] = {
			m_stat[0].bestBid.price
				* m_stat[1].bestBid.price
				* m_stat[2].bestAsk.price,
			m_stat[2].bestBid.price
				* m_stat[1].bestAsk.price
				* m_stat[0].bestAsk.price,
		};
		
		if (opportunite[0] <= 1.0 && opportunite[1] <= 1.0) {
			return;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Detecting current rising / falling:

		const auto &isRising = [this](size_t pair, size_t period) {
			const auto &stat = m_stat[pair];
			const auto &statData = stat.service->GetData(stat.bestBid.source);
			const auto &periodData = period == 0
				?	statData.current
				:	period == 1
					?	statData.prev1
					:	statData.prev2;
			return
				periodData.theo > periodData.emaFast
				&& periodData.emaFast > periodData.emaSlow;
		};

		const auto &isFalling = [this](size_t pair, size_t period) {
			const auto &stat = m_stat[pair];
			const auto &statData = stat.service->GetData(stat.bestAsk.source);
			const auto &periodData = period == 0
				?	statData.current
				:	period == 1
					?	statData.prev1
					:	statData.prev2;
			return
				periodData.theo < periodData.emaFast
				&& periodData.emaFast < periodData.emaSlow;
		};

		/* pair, period, rising , falling */
		bool risingFalling[3][3][2] = {};
		for (size_t pair = 0; pair < m_stat.size(); ++pair) {
			for (size_t period = 0; period < 3; ++period) {
				risingFalling[pair][period][0] = isRising(pair, period);
				risingFalling[pair][period][1] = isFalling(pair, period);
			}
		}

		const bool isBuy[3] = {
			risingFalling[0][0][0],
			!risingFalling[1][0][1],
			!risingFalling[0][0][0]
		};

		////////////////////////////////////////////////////////////////////////////////
		// Scenarios:
		
		size_t scenarios[3] = {};
		for (size_t pair = 0; pair < 3; ++pair) {
			if (
					(isBuy[pair]
						&& risingFalling[pair][0][0]
						&& risingFalling[pair][1][0]
						&& risingFalling[pair][2][0])
					|| (!isBuy[pair]
						&& risingFalling[pair][0][1]
						&& risingFalling[pair][1][1]
						&& risingFalling[pair][2][1])) {
				scenarios[pair] = 1;
			} else if (
					(!isBuy[pair]
						&& risingFalling[pair][0][0]
						&& risingFalling[pair][1][0]
						&& risingFalling[pair][2][0])
					|| (isBuy[pair]
						&& risingFalling[pair][0][1]
						&& risingFalling[pair][1][1]
						&& risingFalling[pair][2][1])) {
				scenarios[pair] = 4;
			}
// #			ifdef BOOST_ENABLE_ASSERT_HANDLER
// 			{
// 				if (scenarios[pair]) {
// 					for (size_t i = 0; i < pair; ++i) {
// 						AssertNe(scenarios[i], scenarios[pair]);
// 					}
// 				}
// 			}
// #			endif
		}
// 		Assert(scenarios[0] == 1 || scenarios[1] == 1 || scenarios[2] == 1);
// 		Assert(scenarios[0] == 4 || scenarios[1] == 4 || scenarios[2] == 4);

		////////////////////////////////////////////////////////////////////////////////

		if (!m_isLogInited) {
			m_strategyLog.Write(
				[&](StrategyLogRecord &record) {
					record
						%	"Horodatage"
						%	"Action"
						%	"Y1"
						%	"Y2";
					foreach (const auto &stat, m_stat) {
						const char *pair = stat.service->GetSecurity(0).GetSymbol().GetSymbol().c_str();
						record
							%	pair % '\0' % " ECN"
							%	pair % '\0' % " Direction"
							%	pair % '\0' % " Order"
/*							%	pair % '\0' % " Inversee"*/
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
			m_isLogInited = true;
		}

		const auto &writePair = [&](size_t pair, StrategyLogRecord &record) {
			const auto &stat = m_stat[pair];
			const auto &ecn = risingFalling[pair][0]
				?	stat.bestBid.source
				:	stat.bestAsk.source;
			const Security &security = stat.service->GetSecurity(ecn);
			record
				%	security.GetSource().GetTag()
				%	(isBuy[pair] ? "BUY" : "SELL")
				%	(scenarios[pair] == 1
						?	"1"
						:	scenarios[pair] == 4 ? "4" : "2-3")
/*				%	(opportunite[0] > 1.0 ? "FALSE" : "TRUE")*/
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
					%	GetContext().GetCurrentTime()
					%	"Opening detected"
					%	opportunite[0]
					%	opportunite[1];
				for (size_t i = 0; i < 3; ++i) {
					writePair(i, record);
					writePair(i, record);
					writePair(i, record);
				}
			});

	}

private:

	//! Updating min/max.
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

	}

protected:

	virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
		//...//
	}

private:

	const size_t m_levelsCount;
	const double m_qty;

	std::ofstream m_strategyLogFile;
	StrategyLog m_strategyLog;
	bool m_isLogInited;

	std::vector<Stat> m_stat;

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

