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
#include "Core/AsyncLog.hpp"

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
	: Base(context, "TriangulationWithDirectionStatService", tag),
	m_bookLevelsCount(
		conf.GetBase().ReadTypedKey<size_t>("General", "book.levels.count")),
	m_isBookLevelsExactly(
		conf.GetBase().ReadBoolKey("General", "book.levels.exactly")),
	m_useAdjustedBookForCalculations(
		conf.GetBase().ReadBoolKey("General", "book.adjust.calculation")),
	m_prev1Duration(
		pt::milliseconds(
			conf.ReadTypedKey<size_t>("prev1_duration_miliseconds"))),
	m_prev2Duration(
		pt::milliseconds(
			conf.ReadTypedKey<size_t>("prev2_duration_miliseconds"))),
	m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow")),
	m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast")),
	m_isLogByPairOn(conf.ReadBoolKey("log.pair")),
	m_pairLog(new ServiceLog),
	m_serviceLog(GetServiceLog(conf)) {
	m_instancies.push_back(this);
	GetLog().Info(
		"Prev1 duration: %1%; Prev2 duration: %2%."
			" Book size: %3% * 2 price levels (%4%)."
			" Use adjusted book for calculations: %5%.",
		m_prev1Duration,
		m_prev2Duration,
		m_bookLevelsCount,
		m_isBookLevelsExactly ? "exactly" : "not exactly",
		m_useAdjustedBookForCalculations ? "yes" : "no");
	if (m_prev2Duration <= m_prev1Duration) {
		throw ModuleError("Prev2 duration can't be equal or less then Prev1");
	}
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
	if (m_data.size() <= dataIndex) {
		m_data.resize(dataIndex + 1);
	}
	Assert(!m_data[dataIndex]);
	auto &source = m_data[dataIndex];
	source.reset(new Source(security, m_emaSpeedSlow, m_emaSpeedFast));
	{
		const Security::Book::Level emptyLevel;
		source->bids.resize(m_bookLevelsCount, emptyLevel);
		source->offers.resize(m_bookLevelsCount, emptyLevel);
	}
	return pt::not_a_date_time;
}

bool StatService::OnBookUpdateTick(
		const Security &security,
		const Security::Book &book,
		const TimeMeasurement::Milestones &) {

	// Method will be called at each book update (add new price level, removed
	// price level or price level updated).

	AssertEq(
		GetSecurity(security.GetSource().GetIndex()).GetSource().GetTag(),
		security.GetSource().GetTag());
	Assert(&GetSecurity(security.GetSource().GetIndex()) == &security);

	if (book.IsAdjusted() && !m_useAdjustedBookForCalculations) {
		return false;
	}

	const Security::Book::Side &bidsBook = book.GetBids();
	const Security::Book::Side &offersBook = book.GetOffers();

#	if defined(DEV_VER) && 0
	{
		if (	
				security.GetSource().GetTag() == "Currenex"
				&& security.GetSymbol().GetSymbol() == "EUR/USD"
				&& (offersBook.GetLevelsCount() >= 4
					|| bidsBook.GetLevelsCount() >= 4)) {
			std::cout
				<< "############################### "
				<< security << " " << security.GetSource().GetTag()
				<< std::endl;
			for (
					size_t i = 0;
					i < std::min(offersBook.GetLevelsCount(), bidsBook.GetLevelsCount());
					++i) {
				std::cout
					<< bidsBook.GetLevel(i).GetQty()
					<< ' ' << bidsBook.GetLevel(i).GetPrice()
					<< "\t\t\t"
					<< offersBook.GetLevel(i).GetPrice()
					<< ' ' << offersBook.GetLevel(i).GetQty()
					<< std::endl;
			}
			std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
		}
	}
#	endif

	if (
			m_isBookLevelsExactly
			&& (bidsBook.GetLevelsCount() < m_bookLevelsCount
				|| offersBook.GetLevelsCount() < m_bookLevelsCount)) {
		return false;
	}

	struct Side {
		Qty qty;
		double vol;
		double avgPrice;
	};
	Side bid = {};
	Side offer = {};

	Data data;

	Source &source = GetSource(security.GetSource().GetIndex());

	////////////////////////////////////////////////////////////////////////////////
	// Current prices and vol:
	{

		bool hasChanges = false;
	
		const auto &sum = [&](
				size_t levelIndex,
				const Security::Book::Side &source,
				Side &result,
				std::vector<Security::Book::Level>::iterator &level) {
			if (source.GetLevelsCount() <= levelIndex) {
				return;
			}
			const auto &sourceLevel = source.GetLevel(levelIndex);
			if (sourceLevel != *level) {
				hasChanges = true;
			}
			*level = sourceLevel;
			result.qty += level->GetQty();
			result.vol += level->GetQty() * level->GetPrice();
			++level;
		};

		const auto realLevelsCount = std::max(
			bidsBook.GetLevelsCount(),
			offersBook.GetLevelsCount());
		const auto actualLinesCount = std::min(
			m_bookLevelsCount,
			realLevelsCount);
		
		data.bids = source.bids;
		auto bidsLevel = data.bids.begin();
		data.offers = source.offers;
		auto offersLevel = data.offers.begin();

		// Calls sum-function for each price level:
		for (size_t i = 0; i < actualLinesCount; ++i) {
			// accumulates prices and vol from current line:
			sum(i, bidsBook, bid, bidsLevel);
			sum(i, offersBook, offer, offersLevel);
		}

		if (!hasChanges) {
			return false;
		}

	}

	data.current.time = book.GetTime();
	
	////////////////////////////////////////////////////////////////////////////////
	// Average prices:
	if (bid.qty != 0) {
		bid.avgPrice = bid.vol / bid.qty;
	}
	if (offer.qty != 0) {
		offer.avgPrice = offer.vol / offer.qty;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	// Theo:
	if (bid.qty != 0 || offer.qty != 0) {
		data.current.theo
			= (bid.avgPrice * offer.qty + offer.avgPrice * bid.qty)
				/ (bid.qty + offer.qty);
		data.current.theo = Round(data.current.theo, security.GetPriceScale());
	}
	
	////////////////////////////////////////////////////////////////////////////////
	// EMAs:
	
	// accumulates values for EMA:
	source.slowEmaAcc(data.current.theo);
	// gets current EMA value:
	data.current.emaSlow
		= Round(accs::ema(source.slowEmaAcc), security.GetPriceScale());

	// accumulates values for EMA:
	source.fastEmaAcc(data.current.theo);
	// gets current EMA value:
	data.current.emaFast
		= Round(accs::ema(source.fastEmaAcc), security.GetPriceScale());

	////////////////////////////////////////////////////////////////////////////////
	// Preparing previous 2 points for actual strategy work:
	source.points.push_back(data.current);
	const auto &p2Time = data.current.time - m_prev2Duration;
	const auto &p1Time = data.current.time - m_prev1Duration;
	auto itP1 = source.points.cend();
	auto itP2 = source.points.cend();
	for (auto it = source.points.cbegin(); it != source.points.cend(); ) {
		if (it->time <= p2Time) {
			AssertLt(it->time, p1Time);
			const auto next = it + 1;
			AssertLe(it->time, next->time);
			if (next == source.points.cend()) {
				// Point only one, and it was a long time ago, so this value
				// was actual at p1 and p2 too.
				itP1
					= itP2
					= it;
				break;
			}
			if (next->time <= p2Time) {
				// Need to remove first point, it already too old for us:
				Assert(it == source.points.cbegin());
				source.points.pop_front();
				it = source.points.cbegin();
				continue;
			}
			// As this time is sutable for p2 - it can be sutable for p1 too:
			itP1
				= itP2
				= it;
		} else if (it->time <= p1Time) {
			// P2, if was, found, now searching only p1:
			Assert(itP2 == source.points.cend() || itP2->time < it->time);
			itP1 = it;
		} else {
			// All found (what exists):
			break;
		}
		++it;
	}
	Assert(
		itP1 == source.points.cend()
		|| itP2 == source.points.cend()
		|| (itP2->time <= itP1->time && itP1->time < data.current.time));
	if (itP1 != source.points.cend()) {
		data.prev1 = *itP1;
	} else {
		// There is no history at start, and only at start:
		AssertEq(data.prev1.time, pt::not_a_date_time);
	}
	if (itP2 != source.points.cend()) {
		data.prev2 = *itP2;
	} else {
		// There is no history at start, and only at start:
		AssertEq(data.prev2.time, pt::not_a_date_time);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Storing new values as actual:
	
	// Locking memory:
	while (source.dataLock.test_and_set(boost::memory_order_acquire));
	// Storing data:
	static_cast<Data &>(source) = data;
	// Unlocking memory:
	source.dataLock.clear(boost::memory_order_release);

	////////////////////////////////////////////////////////////////////////////////
	
	// Write pre-trade log:
	LogState(security.GetSource());

	// If this method returns true - strategy will be notified about actual data
	// update:
	return !IsZero(data.prev2.theo);

}

void StatService::InitLog(
		ServiceLog &log,
		std::ofstream &file,
		const std::string &suffix)
		const {

	Assert(!log.IsEnabled());

	const pt::ptime &now = GetContext().GetStartTime();
	boost::format fileName(
		"pretrade_%1%%2$02d%3$02d_%4$02d%5$02d%6$02d_%7%.csv");
	fileName
		% now.date().year()
		% now.date().month().as_number()
		% now.date().day()
		% now.time_of_day().hours()
		% now.time_of_day().minutes()
		% now.time_of_day().seconds()
		% suffix;
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

StatService::ServiceLog & StatService::GetServiceLog(
		const IniSectionRef &conf)
		const {
	static std::ofstream file;
	static ServiceLog instance;
	if (conf.ReadBoolKey("log.full") && !instance.IsEnabled()) {
		InitLog(instance, file, "full");
	}
	return instance;
}

void StatService::LogState(
		const MarketDataSource &mds)
		const {

	static bool isLogHeadInited = false;
			
	if (!isLogHeadInited) {
	
		const auto writeLogHead = [&](ServiceLogRecord &record) {
			record % ' ';
			foreach (const auto &s, m_data) {
				record % s->security->GetSource().GetTag().c_str();
			}
			record
				% '\n'
				% "fastEMA" % m_emaSpeedFast % '\n'
				% "slowEMA" % m_emaSpeedSlow % '\n'
				% "Prev1" % m_prev1Duration % '\n'
				% "Prev2" % m_prev2Duration % '\n';
		};
		
		m_serviceLog.Write(writeLogHead);

		isLogHeadInited = true;

	}

	if (m_isLogByPairOn) {

		InitLog(
			*m_pairLog,
			m_pairLogFile,
			SymbolToFileName(
				m_data.front()->security->GetSymbol().GetSymbol()));

		const auto writeLogHead = [&](ServiceLogRecord &record) {
			record
				% "time"
				% "ECN"
				% "VWAP"			
				% "VWAP prev1"		
				% "VWAP prev2"		
				% "EMA fast"		
				% "EMA fast prev1"	
				% "EMA fast prev2"	
				% "EMA slow"		
				% "EMA slow prev1"	
				% "EMA slow prev2";
			for (size_t i = 4; i > 0; --i) {
				record
					% (boost::format("bid %1% price")	% i).str()
					% (boost::format("bid %1% qty")	% i).str();
			}
			for (size_t i = 1; i <= 4; ++i) {
				record
					% (boost::format("offer %1% price")	% i).str()
					% (boost::format("offer %1% qty")	% i).str();
			}
		};
		m_pairLog->Write(writeLogHead);

		m_isLogByPairOn = false;

	}

	{

		const auto &write = [&](ServiceLogRecord &record) {
			record % GetContext().GetCurrentTime() % mds.GetTag().c_str(); 
			const Data &data = GetData(mds.GetIndex());
			record
				% data.current.theo
				% data.prev1.theo
				% data.prev2.theo
				% data.current.emaFast
				% data.prev1.emaFast
				% data.prev2.emaFast
				% data.current.emaSlow
				% data.prev1.emaSlow
				% data.prev2.emaSlow;
			foreach_reversed (const auto &line, data.bids) {
				record % line.GetPrice() % line.GetQty();
			}
			foreach (const auto &line, data.offers) {
				record % line.GetPrice() % line.GetQty();
			}
		};

		m_pairLog->Write(write);

	}

	{

		const auto writeValue = [&](
				const StatService *s,
				ServiceLogRecord &record) {
			const auto &mdsCount = GetContext().GetMarketDataSourcesCount();
			const char *const security
				= s->GetSecurity(0).GetSymbol().GetSymbol().c_str();
			record % security % '\0' % " VWAP";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.theo;
			}
			record % '\n' % security % '\0' % " VWAP Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.theo;
			}
			record % '\n' % security % '\0' % " VWAP Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.theo;
			}
			record % '\n' % security % '\0' % " fastEMA";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.emaFast;
			}
			record % '\n' % security % '\0' % " fastEMA Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.emaFast;
			}
			record % '\n' % security % '\0' % " fastEMA Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.emaFast;
			}
			record % '\n' % security % '\0' % " slowEMA";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.current.emaSlow;
			}
			record % '\n' % security % '\0' % " slowEMA Prev1";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev1.emaSlow;
			}
			record % '\n' % security % '\0' % " slowEMA Prev2";
			for (size_t i = 0; i < mdsCount; ++i) {
				const Data &data = s->GetData(i);
				record % data.prev2.emaSlow;
			}
			record % '\n';
		};

		const auto &write = [&](ServiceLogRecord &record) {
			foreach (const StatService *s, m_instancies) {
				writeValue(s, record);
			}
		};

		m_serviceLog.Write(write);

	}

}

void StatService::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);

	const auto newEmaSpeedSlow = conf.ReadTypedKey<double>("ema_speed_slow");
	const auto newEmaSpeedFast = conf.ReadTypedKey<double>("ema_speed_fast");
	GetLog().Info(
		"Set EMA speed: Slow %1% -> %2%; Fast %3% -> %4%.",
		m_emaSpeedSlow,
		newEmaSpeedSlow,
		m_emaSpeedFast,
		newEmaSpeedFast);
	m_emaSpeedSlow = newEmaSpeedSlow;
	m_emaSpeedFast = newEmaSpeedFast;

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
