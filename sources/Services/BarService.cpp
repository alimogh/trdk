/**************************************************************************
 *   Created: 2012/11/15 23:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "BarService.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

//////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Tag>
	struct TagToExtractor {
		//...//
	};

	template<>
	struct TagToExtractor<accs::tag::max> {
		typedef accs::tag::max Tag;
		static const accs::extractor<Tag> & Get() {
			return accs::max;
		}
	};

	template<>
	struct TagToExtractor<accs::tag::min> {
		typedef accs::tag::min Tag;
		static const accs::extractor<Tag> & Get() {
			return accs::min;
		}
	};

	template<typename StatT, size_t fieldOffset>
	class StatAccumulator : public StatT {

	public:

		typedef StatT Base;
		typedef typename Base::ValueType ValueType;

	private:

		typedef accs::accumulator_set<
			ValueType,
			accs::stats<
					accs::tag::min,
					accs::tag::max>>
				Accumulator;

	public:

		explicit StatAccumulator(const BarService &source, size_t size) {
			size = std::min(size, source.GetSize());
			for (size_t i = 0; i < size; ++i) {
				const int8_t *const bar
					= reinterpret_cast<const int8_t *>(
						&source.GetBarByReversedIndex(i));
				m_accumulator(
					*reinterpret_cast<const ValueType *>(
						bar + fieldOffset));
			}
		}

		virtual ~StatAccumulator() {
			//...//
		}

	public:

		virtual ValueType GetMax() const {
			return GetValue<accs::tag::max>();
		}

		virtual ValueType GetMin() const {
			return GetValue<accs::tag::min>();
		}

	private:

		template<typename Tag>
		ValueType GetValue() const {
			typedef TagToExtractor<Tag> Extractor;
			return Extractor::Get()(m_accumulator);
		}

	private:

		Accumulator m_accumulator;

	};

	const char csvDelimeter = ',';

}

////////////////////////////////////////////////////////////////////////////////

BarService::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

BarService::BarDoesNotExistError::BarDoesNotExistError(const char *what) throw()
		: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

BarService::Bar::Bar()
		: maxAskPrice(0),
		openAskPrice(0),
		closeAskPrice(0),
			
		minBidPrice(0),
		openBidPrice(0),
		closeBidPrice(0),

		openTradePrice(0),
		closeTradePrice(0),
			
		highTradePrice(0),
		lowTradePrice(0),

		tradingVolume(0) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

BarService::Stat::Stat() {
	//...//
}
BarService::Stat::~Stat() {
	//...//
}

BarService::ScaledPriceStat::ScaledPriceStat() {
	//...//
}
BarService::ScaledPriceStat::~ScaledPriceStat() {
	//...//
}

BarService::QtyStat::QtyStat() {
	//...//
}
BarService::QtyStat::~QtyStat() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class BarService::Implementation : private boost::noncopyable {

public:

	enum Units {
		UNITS_SECONDS,
		UNITS_MINUTES,
		UNITS_HOURS,
		UNITS_DAYS,
		UNITS_WEEKS,
		numberOfUnits
	};

	typedef std::map<size_t, Bar> Bars;

	struct BarsLog {
		std::ofstream file;
		fs::path path;
	};

public:

	BarService &m_service;

	volatile long m_size;

	std::string m_unitsStr;
	Units m_units;
	
	std::string m_barSizeStr;
	long m_barSizeUnits;
	pt::time_duration m_barSize;

	Bars m_bars;
	Bar *m_currentBar;
	boost::posix_time::ptime m_currentBarEnd;

	std::unique_ptr<BarsLog> m_barsLog;

public:

	explicit Implementation(
				BarService &service,
				const IniFileSectionRef &configuration)
			: m_service(service),
			m_size(0),
			m_currentBar(nullptr),
			m_barsLog(nullptr) {

		{
			const std::string sizeStr = configuration.ReadKey("size");
			const boost::regex expr(
				"(\\d+)\\s+([a-z]+)",
				boost::regex_constants::icase);
			boost::smatch what;
			if (!boost::regex_match(sizeStr , what, expr)) {
				m_service.GetLog().Error(
					"Wrong size size format: \"%1%\". Example: \"5 minutes\".",
					sizeStr);
				throw Error("Wrong bar size settings");
			}
			m_barSizeStr = what.str(1);
			m_unitsStr = what.str(2);
		}

		m_barSizeUnits
			= boost::lexical_cast<decltype(m_barSizeUnits)>(m_barSizeStr);
		if (m_barSizeUnits <= 0) {
			m_service.GetLog().Error(
				"Wrong size specified: \"%1%\"."
					" Size can't be zero or less.",
				m_barSizeStr);
			throw Error("Wrong bar size settings");
		}

		static_assert(numberOfUnits == 5, "Units list changed.");
		if (boost::iequals(m_unitsStr, "seconds")) {
			if (60 % m_barSizeUnits) {
				m_service.GetLog().Error(
					"Wrong size specified: \"%1%\"."
						" Must be a multiple of 1 minute.",
					m_barSizeStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_SECONDS;
			m_barSize = boost::posix_time::seconds(m_barSizeUnits);
		} else if (boost::iequals(m_unitsStr, "minutes")) {
			if (60 % m_barSizeUnits) {
				m_service.GetLog().Error(
					"Wrong size specified: \"%1%\"."
						" Must be a multiple of 1 hour.",
					m_barSizeStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_MINUTES;
			m_barSize = boost::posix_time::minutes(m_barSizeUnits);
		} else if (boost::iequals(m_unitsStr, "hours")) {
			m_units = UNITS_HOURS;
			m_barSize = boost::posix_time::hours(m_barSizeUnits);
		} else if (boost::iequals(m_unitsStr, "days")) {
			m_units = UNITS_DAYS;
			m_barSize = boost::posix_time::hours(m_barSizeUnits * 24);
			throw Error("Days units doesn't yet implemented");
		} else if (boost::iequals(m_unitsStr, "weeks")) {
			m_units = UNITS_WEEKS;
			m_barSize = boost::posix_time::hours((m_barSizeUnits * 24) * 7);
			throw Error("Weeks units doesn't yet implemented");
		} else {
			m_service.GetLog().Error(
				"Wrong size specified: \"%1%\". Unknown units."
					"Supported: seconds, minutes, hours, days and weeks.",
				m_unitsStr);
			throw Error("Wrong bar size settings");
		}

		ReopenLog(configuration);

		m_service.GetLog().Info("Stated with size \"%1%\".", m_barSize);

	}

	void ReopenLog(const IniFileSectionRef &configuration) {

		const std::string logType = configuration.ReadKey("log", "none");
		if (boost::iequals(logType, "none")) {
			if (m_barsLog) {
				m_barsLog.reset();
				m_service.GetLog().Info("Logging disabled.");
			}
			return;
		} else if (!boost::iequals(logType, "csv")) {
			m_service.GetLog().Error(
				"Wrong log type settings: \"%1%\". Unknown type."
					" Supported: none and CSV.",
				logType);
			throw Error("Wrong bars log type");
		}

		std::unique_ptr<BarsLog> log(new BarsLog);
		log->path = SymbolToFilePath(
			//! @todo Use context log dir
			Defaults::GetBarsDataLogDir(),
			//! @todo Generate unique filename
			(boost::format("%1%_%2%")
					% m_unitsStr
					% m_barSizeStr)
				.str(),
			logType);
		if (m_barsLog && m_barsLog->path == log->path) {
			return;
		}

		const bool isNew = !fs::exists(log->path);
		if (isNew) {
			fs::create_directories(log->path.branch_path());
		}
		log->file.open(
			log->path.string(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!log->file) {
			m_service.GetLog().Error("Failed to open log file %1%", log->path);
			throw Error("Failed to open log file");
		}
		if (isNew) {
			log->file
				<< "Date"
				<< csvDelimeter << "Time"
				<< csvDelimeter << "Open"
				<< csvDelimeter << "High"
				<< csvDelimeter << "Low"
				<< csvDelimeter << "Close"
				<< csvDelimeter << "Volume"
				<< std::endl;
		}
		log->file << std::setfill('0');

		m_service.GetLog().Info("Logging into %1%.", log->path);
		std::swap(log, m_barsLog);

	}

	void LogCurrentBar() const {
		if (!m_barsLog || !m_currentBar) {
			return;
		}
		const auto barStartTime = m_currentBarEnd - m_barSize;
		{
			const auto date = m_currentBar->time.date();
			m_barsLog->file
				<< date.year()
				<< std::setw(2) << date.month().as_number()
				<< std::setw(2) << date.day();
		}
		{
			const auto time = m_currentBar->time.time_of_day();
			m_barsLog->file
				<< csvDelimeter
				<< std::setw(2) << time.hours()
				<< std::setw(2) << time.minutes()
				<< std::setw(2) << time.seconds();
		}
		m_barsLog->file
			<< csvDelimeter << m_currentBar->openTradePrice
			<< csvDelimeter << m_currentBar->highTradePrice
			<< csvDelimeter << m_currentBar->lowTradePrice
			<< csvDelimeter << m_currentBar->closeTradePrice
			<< csvDelimeter << m_currentBar->tradingVolume
			<< std::endl;
	}

	void GetBarTimePoints(
				const pt::ptime &tradeTime,
				pt::ptime &startTime,
				pt::ptime &endTime)
			const {
		AssertNe(pt::not_a_date_time, tradeTime);
		const auto time = tradeTime.time_of_day();
		static_assert(numberOfUnits == 5, "Units list changed.");
		switch (m_units) {
			case UNITS_SECONDS:
				endTime
					= pt::ptime(tradeTime.date())
					+ pt::hours(time.hours())
					+ pt::minutes(time.minutes())
					+ pt::seconds(
						((time.seconds() / m_barSizeUnits) + 1)
							* m_barSizeUnits);
				startTime = endTime - pt::seconds(m_barSizeUnits);
				break;
			case UNITS_MINUTES:
				endTime
					= pt::ptime(tradeTime.date())
					+ pt::hours(time.hours())
					+ pt::minutes(
						((time.minutes() / m_barSizeUnits) + 1)
							* m_barSizeUnits);
				startTime = endTime - pt::minutes(m_barSizeUnits);
				break;
			case UNITS_HOURS:
				endTime
					= pt::ptime(tradeTime.date())
					+ pt::hours(
						((time.hours() / m_barSizeUnits) + 1)
							* m_barSizeUnits);
				startTime = endTime - pt::hours(m_barSizeUnits);
				break;
			case UNITS_DAYS:
				//! @todo Implement days bar service
				throw Error("Days units doesn't yet implemented");
			case UNITS_WEEKS:
				//! @todo Implement days bar service
				throw Error("Weeks units doesn't yet implemented");
			default:
				AssertFail("Unknown units type");
				throw Error("Unknown bar service units type");
		}
	}

	template<typename Pred>
	bool ContinueBar(const Pred &pred) {
		Assert(!m_bars.empty());
		AssertNe(pt::not_a_date_time, m_currentBarEnd);
		pred(*m_currentBar);
		return false;
	}

	template<typename Pred>
	bool StartNewBar(const pt::ptime &time, const Pred &pred) {
		LogCurrentBar();
		m_currentBar = &m_bars[size_t(m_size)];
		GetBarTimePoints(time, m_currentBar->time, m_currentBarEnd);
		pred(*m_currentBar);
		Interlocking::Increment(m_size);
		AssertEq(size_t(m_size), m_bars.size());
		return m_size > 1;
	}

	template<typename Pred>
	bool AppendStat(const pt::ptime &time, const Pred &pred) {
		AssertLe(size_t(m_size), m_bars.size());
		return m_currentBar && m_currentBarEnd > time
			?	ContinueBar(pred)
			:	StartNewBar(time, pred);
	}

	bool OnLevel1Tick(
				const Security &security,
				const pt::ptime &time,
				const Level1TickValue &value) {
		switch (value.type) {
			case LEVEL1_TICK_BID_PRICE:
			case LEVEL1_TICK_ASK_PRICE:
				break;
			default:
				return false;
		};
		return AppendStat(
			time,
			[&](Bar &bar) {
			
				switch (value.type) {
			
					default:
						AssertEq(LEVEL1_TICK_BID_PRICE, value.type);
						return;
			
					case LEVEL1_TICK_BID_PRICE:
						
						if (!bar.openBidPrice) {
							bar.openBidPrice = value.value.bidPrice;
						}
						bar.minBidPrice = bar.minBidPrice
							?	std::min(
									bar.minBidPrice,
									value.value.bidPrice)
							:	bar.openBidPrice;
						bar.closeBidPrice = value.value.bidPrice;
						
						if (!bar.openAskPrice) {
							AssertEq(0, bar.maxAskPrice);
							AssertEq(0, bar.closeAskPrice);
							if (m_size > 0) {
								bar.openAskPrice = m_bars[m_size - 1].closeAskPrice;
							}
							if (!bar.openAskPrice) {
								bar.openAskPrice = security.GetAskPriceScaled();
							}
							bar.maxAskPrice = bar.openAskPrice;
							bar.closeAskPrice = bar.openAskPrice;
						} else {
							AssertNe(0, bar.maxAskPrice);
							AssertNe(0, bar.closeAskPrice);
						}
						
						break;
			
					case LEVEL1_TICK_ASK_PRICE:

						if (!bar.openAskPrice) {
							bar.openAskPrice = value.value.askPrice;
						}
						bar.maxAskPrice = std::max(
							bar.maxAskPrice,
							value.value.askPrice);
						bar.closeAskPrice = value.value.askPrice;

						if (!bar.openBidPrice) {
							AssertEq(0, bar.minBidPrice);
							AssertEq(0, bar.closeBidPrice);
							if (m_size > 0) {
								bar.openBidPrice = m_bars[m_size - 1].closeBidPrice;
							}
							if (!bar.openBidPrice) {
								bar.openBidPrice = security.GetBidPriceScaled();
							}
							bar.minBidPrice = bar.openBidPrice;
							bar.closeBidPrice = bar.openBidPrice;
						} else {
							AssertNe(0, bar.minBidPrice);
							AssertNe(0, bar.closeBidPrice);
						}

						break;
			
				}

			});
	}

	bool OnNewTrade(
				const Security &,
				const pt::ptime &time,
				ScaledPrice price,
				Qty qty) {
		return AppendStat(
			time,
			[&](Bar &bar) {
				if (!bar.openTradePrice) {
					AssertEq(0, bar.highTradePrice);
					AssertEq(0, bar.lowTradePrice);
					AssertEq(0, bar.closeTradePrice);
					AssertEq(0, bar.tradingVolume);
					AssertNe(0, price);
					bar.openTradePrice = price;
				}
				bar.highTradePrice = std::max(bar.highTradePrice, price);
				bar.lowTradePrice = bar.lowTradePrice
					?	std::min(bar.lowTradePrice, price)
					:	price;
				bar.closeTradePrice = price;
				bar.tradingVolume += qty;
			});
	}

	template<typename Stat>
	boost::shared_ptr<typename Stat> CreateStat(size_t size) const {
		return boost::shared_ptr<typename Stat>(new Stat(m_service, size));
	}

};

//////////////////////////////////////////////////////////////////////////

BarService::BarService(
			Context &context,
			const std::string &tag,
			const IniFileSectionRef &configuration)
		: Service(context, "BarsService", tag) {
	m_pimpl = new Implementation(*this, configuration);
}

BarService::~BarService() {
	delete m_pimpl;
}

pt::ptime BarService::OnSecurityStart(const Security &) {
	return boost::get_system_time() - GetBarSize();
}

bool BarService::OnLevel1Tick(
			const Security &security,
			const pt::ptime &time,
			const Level1TickValue &value) {
	return m_pimpl->OnLevel1Tick(security, time, value);
}

bool BarService::OnNewTrade(
			const Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide) {
	return m_pimpl->OnNewTrade(security, time, price, qty);
}

const pt::time_duration & BarService::GetBarSize() const {
	return m_pimpl->m_barSize;
}

void BarService::UpdateAlogImplSettings(
			const IniFileSectionRef &configuration) {
	m_pimpl->ReopenLog(configuration);
}

const BarService::Bar & BarService::GetBar(size_t index) const {
	if (index >= GetSize()) {
		throw BarDoesNotExistError(
			IsEmpty()
				?	"BarService is empty"
				:	"Index is out of range of BarService");
	}
	const Lock lock(GetMutex());
	const auto pos = m_pimpl->m_bars.find(index);
	Assert(pos != m_pimpl->m_bars.end());
	return pos->second;
}

const BarService::Bar & BarService::GetBarByReversedIndex(
			size_t index)
		const {
	if (index >= GetSize()) {
		throw BarDoesNotExistError(
			IsEmpty()
				?	"BarService is empty"
				:	"Index is out of range of BarService");
	}
	const Lock lock(GetMutex());
	const auto pos = m_pimpl->m_bars.find(m_pimpl->m_size - index - 1);
	Assert(pos != m_pimpl->m_bars.end());
	return pos->second;
}

const BarService::Bar & BarService::GetLastBar() const {
	return GetBarByReversedIndex(0);
}

size_t BarService::GetSize() const {
	return m_pimpl->m_size;
}

bool BarService::IsEmpty() const {
	return m_pimpl->m_size == 0;
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetOpenPriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::openTradePrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetClosePriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::closeTradePrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetHighPriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::highTradePrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetLowPriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::lowTradePrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::QtyStat> BarService::GetTradingVolumeStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			QtyStat,
			offsetof(BarService::Bar, BarService::Bar::tradingVolume)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateBarsService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new BarService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateBarsService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new BarService(context, tag, configuration));
	}
#endif

//////////////////////////////////////////////////////////////////////////
