/**************************************************************************
 *   Created: 2014/11/30 05:21:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/TradingLog.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class ServiceLogRecord : public AsyncLogRecord {
	public:
		explicit ServiceLogRecord(
				const Log::Time &time,
				const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}
	public:
		const ServiceLogRecord & operator >>(std::ostream &os) const {
			Dump(os, ",");
			return *this;
		}
	};

	inline std::ostream & operator <<(
			std::ostream &os,
			const ServiceLogRecord &record) {
		record >> os;
		return os;
	}

	class ServiceLogOutStream : private boost::noncopyable {
	public:
		void Write(const ServiceLogRecord &record) {
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
			ServiceLogRecord,
			ServiceLogOutStream,
			TRDK_CONCURRENCY_PROFILE>
		ServiceLogBase;

}

class StatService::ServiceLog : private ServiceLogBase {

public:

	typedef ServiceLogBase Base;

public:

	using Base::IsEnabled;
	using Base::EnableStream;

	template<typename FormatCallback>
	void Write(const FormatCallback &formatCallback) {
		FormatAndWrite(formatCallback);
	}

};

////////////////////////////////////////////////////////////////////////////////

std::vector<StatService *> StatService::m_instancies;

StatService::StatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirectionStatService", tag)
	, m_bookLevelsCount(
		conf.GetBase().ReadTypedKey<size_t>("General", "book.levels.count"))
	, m_period1(pt::milliseconds(500))
	, m_period2(pt::seconds(2))
	, m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow"))
	, m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast"))
	, m_slowEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedSlow)
	, m_fastEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedFast)
	, m_isLogByPairEnabled(conf.ReadBoolKey("log.pair"))
	, m_pairLog(new ServiceLog)
	, m_pairLogNumberOfRows(0) {

	m_stat = {};

	m_instancies.push_back(this);

	if (m_bookLevelsCount < 1) {
		throw ModuleError("Book Size can't be \"zero\"");
	}

	m_aggregatedBidsCache.resize(m_bookLevelsCount);
	m_aggregatedAsksCache.resize(m_bookLevelsCount);

}

StatService::~StatService() {
	try {
		const auto &pos = std::find(
			m_instancies.begin(),
			m_instancies.end(),
			this);
		Assert(pos != m_instancies.end());
		m_instancies.erase(pos);
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

pt::ptime StatService::OnSecurityStart(const Security &security) {
	const auto &dataIndex = security.GetSource().GetIndex();
	if (m_sources.size() <= dataIndex) {
		m_sources.resize(dataIndex + 1, {});
	}
	Assert(!m_sources[dataIndex].security);
	m_sources[dataIndex].security = &security;
	return pt::not_a_date_time;
}

bool StatService::OnBookUpdateTick(
		const Security &security,
		const Security::Book &book,
		const TimeMeasurement::Milestones &) {

	// Method will be called at each book update (add new price level, removed
	// price level or price level updated).

	// Store each book update as "last book" to calculate stat by each book
	// tick.
	{
	
		Source &source = m_sources[security.GetSource().GetIndex()];
		Assert(source.security == &security);
		Assert(
			source.time == pt::not_a_date_time
			|| source.time <= book.GetTime());
	
		if (
				book.GetBids().GetSize() < m_bookLevelsCount
				|| book.GetAsks().GetSize() < m_bookLevelsCount) {
			if (source.isUsed) {
				GetTradingLog().Write(
					"insufficient book data\t%1%\t%2%\t%3%x%4%",
					[&](TradingRecord &record) {
						record
							% source.security->GetSymbol().GetSymbol()
							% source.security->GetSource().GetTag()
							% book.GetBids().GetSize()
							% book.GetAsks().GetSize();
					});
				source.isUsed = false;
			}
			// TRDK-268 Ignore the ECN with less than 4 levels
			// (but use best price).
			return false;
		} else if (!source.isUsed) {
			GetTradingLog().Write(
				"book data received\t%1%\t%2%\t%3%x%4%",
				[&](TradingRecord &record) {
					record
						% source.security->GetSymbol().GetSymbol()
						% source.security->GetSource().GetTag()
						% book.GetBids().GetSize()
						% book.GetAsks().GetSize();
				});
			source.isUsed = true;
		}

		source.time = book.GetTime();
		source.bidsBook = book.GetBids();
		source.asksBook = book.GetAsks();

	}

	////////////////////////////////////////////////////////////////////////////////
	// Aggregating books, preparing opportunity statistics:

	// Aggregating books:
	for (size_t i = 0; i < m_bookLevelsCount; ++i) {

		PriceLevel &bid = m_aggregatedBidsCache[i];
		bid.Reset();

		PriceLevel &ask = m_aggregatedAsksCache[i];
		ask.Reset();

		foreach (const Source &source, m_sources) {

			if (!source.isUsed) {
				continue;
			}

			{
				const auto &sourceBid = source.bidsBook.GetLevel(i);
				Assert(!IsZero(sourceBid.GetPrice()));
				Assert(!IsZero(sourceBid.GetQty()));
				if (bid.price <= sourceBid.GetPrice()) {
					if (IsEqual(bid.price, sourceBid.GetPrice())) {
						bid.qty += sourceBid.GetQty();
					} else {
						bid.price = sourceBid.GetPrice();
						bid.qty = sourceBid.GetQty();
					}
				}
			}

			{
				const auto &sourceAsk = source.asksBook.GetLevel(i);
				Assert(!IsZero(sourceAsk.GetPrice()));
				Assert(!IsZero(sourceAsk.GetQty()));
				if (IsZero(ask.price) || ask.price > sourceAsk.GetPrice()) {
					ask.price = sourceAsk.GetPrice();
					ask.qty = sourceAsk.GetQty();
				} else if (IsEqual(ask.price, sourceAsk.GetPrice())) {
					ask.qty += sourceAsk.GetQty();
				}
			}

		}

		if (IsZero(bid.qty) || IsZero(ask.qty)) {
			return false;
		}
		Assert(!IsZero(bid.price));
		Assert(!IsZero(ask.price));

	}

	// Sorting:
	std::sort(
		m_aggregatedBidsCache.begin(),
		m_aggregatedBidsCache.end(),
		std::greater<PriceLevel>());
	std::sort(m_aggregatedAsksCache.begin(), m_aggregatedAsksCache.end());

	// Inversing and calculation stat:
	struct SideStat {
		Qty qty;
		double vol;
		SideStat & operator <<(PriceLevel &rhs) {
			Assert(!IsZero(rhs.price));
			Assert(!IsZero(rhs.qty));
			qty += rhs.qty;
			vol += rhs.qty * rhs.price;
			return *this;
		}
	};
	SideStat bidStat = {};
	SideStat askStat = {};
	for (size_t i = 0; i < m_bookLevelsCount; ++i) {
		if (m_aggregatedBidsCache[i] > m_aggregatedAsksCache[i]) {
			Assert(m_aggregatedAsksCache[i] < m_aggregatedBidsCache[i]);
			bidStat << m_aggregatedAsksCache[i];
			askStat << m_aggregatedBidsCache[i];
#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				std::swap(m_aggregatedBidsCache[i], m_aggregatedAsksCache[i]);
#			endif
		} else {
			bidStat << m_aggregatedBidsCache[i];
			askStat << m_aggregatedAsksCache[i];
		}
	}
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
	{
		foreach (const auto &i, m_aggregatedBidsCache) {
			foreach (const auto &j, m_aggregatedAsksCache) {
				AssertLe(i.price, j.price);
			}
		}
		foreach (const auto &i, m_aggregatedAsksCache) {
			foreach (const auto &j, m_aggregatedBidsCache) {
				AssertGe(i.price, j.price);
			}
		}
	}
#	endif

	Assert(!IsZero(bidStat.vol));
	Assert(!IsZero(askStat.vol));
	Assert(!IsZero(bidStat.qty));
	Assert(!IsZero(askStat.qty));

	////////////////////////////////////////////////////////////////////////////////
	// Time:

	UpdateNumberOfUpdates(book);

	Point point = {book.GetTime()};
	foreach (const Source &source, m_sources) {
		if (!source.isUsed) {
			continue;
		}
		if (source.time != pt::not_a_date_time && source.time > point.time) {
			point.time = source.time;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// VWAP (see TRDK-267 about precision):

	point.vwapBid = bidStat.vol / bidStat.qty;
	point.vwapAsk = askStat.vol / askStat.qty;

	////////////////////////////////////////////////////////////////////////////////
	// Theo (see TRDK-267 about precision):
	
	point.theo
		= ((point.vwapBid * bidStat.qty) + (point.vwapAsk * askStat.qty))
			/ (bidStat.qty + askStat.qty);

	////////////////////////////////////////////////////////////////////////////////
	// EMAs:
	
	// accumulates values for EMA:
	m_slowEmaAcc(point.theo);
	// gets current EMA value (see TRDK-267 about precision):
	point.emaSlow = accs::ema(m_slowEmaAcc), security.GetPriceScale();

	// accumulates values for EMA:
	m_fastEmaAcc(point.theo);
	// gets current EMA value (see TRDK-267 about precision):
	point.emaFast = accs::ema(m_fastEmaAcc), security.GetPriceScale();

	////////////////////////////////////////////////////////////////////////////////
	// Preparing previous 2 points for actual strategy work:
	
	m_history.push_back(point);
	const auto &p2Time = point.time - m_period2;
	const auto &p1Time = point.time - m_period1;
	AssertGt(point.time, p1Time);
	AssertGt(p1Time, p2Time);
	auto itP1 = m_history.cend();
	auto itP2 = m_history.cend();
	for (auto it = m_history.cbegin(); it != m_history.cend(); ) {
		if (it->time <= p2Time) {
			AssertLt(it->time, p1Time);
			const auto next = std::next(it);
			AssertLe(it->time, next->time);
			if (next == m_history.cend()) {
				// Point only one, and it was a long time ago, so this value
				// was actual at p1 and p2 too.
				itP1
					= itP2
					= it;
				break;
			}
			if (next->time <= p2Time) {
				// Need to remove first point, it already too old for us:
				Assert(it == m_history.cbegin());
				m_history.pop_front();
				it = m_history.cbegin();
				continue;
			}
			// As this time is suitable for p2 - it can be suitable for p1 too:
			itP1
				= itP2
				= it;
		} else if (it->time <= p1Time) {
			// P2, if was, found, now searching only p1:
			Assert(itP2 == m_history.cend() || itP2->time < it->time);
			itP1 = it;
		} else {
			// All found (what exists):
			break;
		}
		++it;
	}
	if (itP2 == m_history.cend()) {
		LogState(false);
		return false;
	}
	Assert(itP1 != m_history.cend());
	Assert(itP2->time <= itP1->time && itP1->time < point.time);
	if (itP1 == itP2) {
		LogState(false);
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Storing calculated data for public access:

	while (m_statMutex.test_and_set(boost::memory_order_acquire));
	m_stat.history[2] = point;
	m_stat.history[1] = *itP1;
	m_stat.history[0] = *itP2;
	m_stat.numberOfUpdates = m_numberOfUpdates.size();
	m_statMutex.clear(boost::memory_order_release);

	////////////////////////////////////////////////////////////////////////////////
	
	// If this method returns true - strategy will be notified about actual data
	// update:
	LogState(true);
	return true;

}

void StatService::UpdateNumberOfUpdates(const Security::Book &book) {

	m_numberOfUpdates.push_back(book.GetTime());
	
	const auto &startTime = book.GetTime() - pt::seconds(30);
	while (
			!m_numberOfUpdates.empty()
			&& m_numberOfUpdates.front() < startTime) {
		m_numberOfUpdates.pop_front();
	}

}

void StatService::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);

	const auto newEmaSpeedSlow = conf.ReadTypedKey<double>("ema_speed_slow");
	const auto newEmaSpeedFast = conf.ReadTypedKey<double>("ema_speed_fast");

	bool reset = false;

	if (m_emaSpeedSlow != m_emaSpeedSlow || m_emaSpeedFast != newEmaSpeedFast) {
		GetLog().Info(
			"Set EMA speed: Slow %1% -> %2%; Fast %3% -> %4%.",
			m_emaSpeedSlow,
			newEmaSpeedSlow,
			m_emaSpeedFast,
			newEmaSpeedFast);
		m_emaSpeedSlow = newEmaSpeedSlow;
		m_emaSpeedFast = newEmaSpeedFast;
		reset = true;
	}


	if (reset) {

		while (m_statMutex.test_and_set(boost::memory_order_acquire));
		m_stat = {};
		m_statMutex.clear(boost::memory_order_release);

		m_history.clear();

		m_slowEmaAcc = EmaAcc(accs::tag::ema_speed::speed = m_emaSpeedSlow);
		m_fastEmaAcc = EmaAcc(accs::tag::ema_speed::speed = m_emaSpeedFast);

	}

}

void StatService::InitLog(
		ServiceLog &log,
		std::ofstream &file,
		const std::string &suffix)
		const {

	Assert(!log.IsEnabled());

	const pt::ptime &now = GetContext().GetStartTime();
	boost::format fileName(
		"pretrade_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d_%7%_%8%.csv");
	fileName
		% now.date().year()
		% now.date().month().as_number()
		% now.date().day()
		% now.time_of_day().hours()
		% now.time_of_day().minutes()
		% now.time_of_day().seconds()
		% suffix
		% GetTag();
	const auto &logPath
		= GetContext().GetSettings().GetLogsDir() / "pretrade" / fileName.str();
	
	GetContext().GetLog().Info("Log: %1%.", logPath);
	fs::create_directories(logPath.branch_path());
	file.open(
		logPath.string().c_str(),
		std::ios::app | std::ios::ate);
	if (!file) {
		throw ModuleError("Failed to open log file");
	}

	log.EnableStream(file);

}

void StatService::LogState(bool isUpdateUsed) const {

	if (m_pairLogNumberOfRows >= 800000) {
		return;
	}
	++m_pairLogNumberOfRows;

	if (m_isLogByPairEnabled) {

		InitLog(
			*m_pairLog,
			m_pairLogFile,
			SymbolToFileName(
				m_sources.front().security->GetSymbol().GetSymbol()));

		const auto writeLogHead = [&](ServiceLogRecord &record) {

			record
				% "Time"
				% "Used"
				% "Point time"
				% "Point time prev 1"
				% "Point time prev 2"
				% "Period start prev 1"
				% "Period start prev 2";

			for (size_t i = 0; i < m_bookLevelsCount; ++i) {
				const auto index = boost::lexical_cast<std::string>(i + 1);
				record
					% (std::string("Bid Price ") + index)
					% (std::string("Bid Size ") + index);
			}
			record % "VWAP Bid" % "VWAP Bid prev 1" % "VWAP Bid prev 2";

			for (size_t i = 0; i < m_bookLevelsCount; ++i) {
				const auto index = boost::lexical_cast<std::string>(i + 1);
				record
					% (std::string("Ask Price ") + index)
					% (std::string("Ask Size ") + index);
			}
			record % "VWAP Ask" % "VWAP Ask prev 1" % "VWAP Ask prev 2";

			record % "Theo" % "Theo prev 1" % "Theo prev 2";

			record % "EMA Slow" % "EMA Slow prev 1" % "EMA Slow prev 2";
			record % "EMA Fast" % "EMA Fast prev 1" % "EMA Fast prev 2";

			record % "Number of updates";

		};
		m_pairLog->Write(writeLogHead);

		m_isLogByPairEnabled = false;

	}

	m_pairLog->Write(
		[&](ServiceLogRecord &record) {
			
			record % GetContext().GetCurrentTime();
			record % (isUpdateUsed ? "yes" : "no");
			foreach_reversed (const Point &i, m_stat.history) {
				record % i.time;
			}
			record % (m_stat.history.back().time - m_period1);
			record % (m_stat.history.back().time - m_period2);

			foreach (const PriceLevel &i, m_aggregatedBidsCache) {
				record % i.price % i.qty;
			}
			foreach_reversed (const Point &i, m_stat.history) {
				record % i.vwapBid;
			}

			foreach (const PriceLevel &i, m_aggregatedAsksCache) {
				record % i.price % i.qty;
			}
			foreach_reversed (const Point &i, m_stat.history) {
				record % i.vwapAsk;
			}

			foreach_reversed (const Point &i, m_stat.history) {
				record % i.theo;
			}

			foreach_reversed (const Point &i, m_stat.history) {
				record % i.emaSlow;
			}
			foreach_reversed (const Point &i, m_stat.history) {
				record % i.emaFast;
			}

			record % m_stat.numberOfUpdates;

		});

}

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Service> CreateTriangulationWithDirectionStatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<Service>(
		new Twd::StatService(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
