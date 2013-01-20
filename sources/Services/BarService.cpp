/**************************************************************************
 *   Created: 2012/11/15 23:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "BarService.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

using namespace Trader;
using namespace Trader::Services;
using namespace Trader::Lib;

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

		explicit StatAccumulator(
					boost::shared_ptr<const BarService> source,
					size_t size) {
			size = std::min(size, source->GetSize());
			for (size_t i = 0; i < size; ++i) {
				const int8_t *const bar
					= reinterpret_cast<const int8_t *>(
						&source->GetBarByReversedIndex(i));
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
	long m_barSize;

	Bars m_bars;
	Bar *m_currentBar;
	boost::posix_time::ptime m_currentBarEnd;

	std::unique_ptr<BarsLog> m_barsLog;

public:

	explicit Implementation(BarService &service, const IniFileSectionRef &ini)
			: m_service(service),
			m_size(0),
			m_currentBar(nullptr),
			m_barsLog(nullptr) {

		{
			const std::string sizeStr = ini.ReadKey("size", false);
			const boost::regex expr(
				"(\\d+)\\s+([a-z]+)",
				boost::regex_constants::icase);
			boost::smatch what;
			if (!boost::regex_match(sizeStr , what, expr)) {
				m_service.GetLog().Error(
					"Wrong size size format: \"%1%\". Example: \"5 minutes\".",
					what.str(0));
				throw Error("Wrong bar size settings");
			}
			m_barSizeStr = what.str(1);
			m_unitsStr = what.str(2);
		}

		m_barSize = boost::lexical_cast<decltype(m_barSize)>(m_barSizeStr);
		if (m_barSize <= 0) {
			m_service.GetLog().Error(
				"Wrong size specified: \"%1%\"."
					" Size can't be zero or less.",
				m_barSizeStr);
			throw Error("Wrong bar size settings");
		}

		if (boost::iequals(m_unitsStr, "seconds")) {
			if (60 % m_barSize) {
				m_service.GetLog().Error(
					"Wrong size specified: \"%1%\". 1 minute must be"
						" a multiple specified size.",
					m_barSizeStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_SECONDS;
		} else if (boost::iequals(m_unitsStr, "minutes")) {
			if (60 % m_barSize) {
				m_service.GetLog().Error(
					"Wrong size specified: \"%1%\". 1 hour must be"
						" a multiple specified size.",
					m_barSizeStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_MINUTES;
		} else if (boost::iequals(m_unitsStr, "hours")) {
			m_units = UNITS_HOURS;
		} else if (boost::iequals(m_unitsStr, "days")) {
			m_units = UNITS_DAYS;
			throw Error("Days units doesn't yet implemented");
		} else {
			m_service.GetLog().Error(
				"Wrong size specified: \"%1%\". Unknown units."
					"Supported: seconds, minutes, hours and days.",
				m_unitsStr);
			throw Error("Wrong bar size settings");
		}

		ReopenLog(ini, false);

		m_service.GetLog().Info("Stated with size %1%.", m_barSize);
	
	}

	char GetCsvDelimeter() const {
		return ',';
	}

	void ReopenLog(const IniFileSectionRef &ini, bool isCritical) {

		const std::string logType = ini.ReadKey("log", false);
		if (boost::iequals(logType, "none")) {
			m_barsLog.reset();
			return;
		} else if (!boost::iequals(logType, "csv")) {
			m_service.GetLog().Error(
				"Wrong log type settings: \"%1%\". Unknown type."
					" Supported: none and CSV.",
				logType);
			if (!isCritical) {
				throw Error("Wrong bars log type");
			}
			return;
		}

		std::unique_ptr<BarsLog> log(new BarsLog);
		log->path = Util::SymbolToFilePath(
			Defaults::GetBarsDataLogDir(),
			(boost::format("%1%_%2%_%3%")
					% m_unitsStr
					% m_barSizeStr
					% m_service.GetSecurity())
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
			if (!isCritical) {
				throw Error("Failed to open log file");
			}
			return;
		}
		if (isNew) {
			log->file
				<< "Date"
				<< GetCsvDelimeter() << "Time"
				<< GetCsvDelimeter() << "Open"
				<< GetCsvDelimeter() << "High"
				<< GetCsvDelimeter() << "Low"
				<< GetCsvDelimeter() << "Close"
				<< GetCsvDelimeter() << "Volume"
				<< std::endl;
		}
		log->file << std::setfill('0');

		m_service.GetLog().Info("Logging bars into %1%.", log->path);
		std::swap(log, m_barsLog);

	}

	pt::time_duration GetBarSize() const {
		static_assert(numberOfOrderSides, "Units list changed.");
		switch (m_units) {
			case UNITS_SECONDS:
				return boost::posix_time::seconds(m_barSize);
			case UNITS_MINUTES:
				return boost::posix_time::minutes(m_barSize);
			case UNITS_HOURS:
				return boost::posix_time::hours(m_barSize);
			case UNITS_DAYS:
				return boost::posix_time::hours(m_barSize * 24);
			default:
				AssertFail("Unknown units type");
				throw Trader::Lib::Exception(
					"Unknown bar service units type");
		}
	}

	void LogCurrentBar() const {
		if (!m_barsLog || !m_currentBar) {
			return;
		}
		const auto barStartTime = m_currentBarEnd - GetBarSize();
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
				<< GetCsvDelimeter()
				<< std::setw(2) << time.hours()
				<< std::setw(2) << time.minutes()
				<< std::setw(2) << time.seconds();
		}
		m_barsLog->file
			<< GetCsvDelimeter() << m_currentBar->openPrice
			<< GetCsvDelimeter() << m_currentBar->highPrice
			<< GetCsvDelimeter() << m_currentBar->lowPrice
			<< GetCsvDelimeter() << m_currentBar->closePrice
			<< GetCsvDelimeter() << m_currentBar->volume
			<< std::endl;
	}

	pt::ptime GetBarEnd(const pt::ptime &tradeTime)
			const {
		AssertNe(pt::not_a_date_time, tradeTime);
		const auto time = tradeTime.time_of_day();
		static_assert(numberOfOrderSides, "Units list changed.");
		switch (m_units) {
			case UNITS_SECONDS:
				return
					pt::ptime(tradeTime.date())
					+ pt::hours(time.hours())
					+ pt::minutes(time.minutes())
					+ pt::seconds(
						((time.seconds() / m_barSize) + 1) * m_barSize);
			case UNITS_MINUTES:
				return
					pt::ptime(tradeTime.date())
					+ pt::hours(time.hours())
					+ pt::minutes(
						((time.minutes() / m_barSize) + 1) * m_barSize);
			case UNITS_HOURS:
				return
					pt::ptime(tradeTime.date())
					+ pt::hours(
						((time.hours() / m_barSize) + 1) * m_barSize);
			case UNITS_DAYS:
				//! @todo Implement days bar service
				throw Error("Days units doesn't yet implemented");
			default:
				AssertFail("Unknown units type");
				throw Exception("Unknown bar service units type");
		}
	}

	bool OnNewTrade(const pt::ptime &time, ScaledPrice price, Qty qty) {
		AssertLe(size_t(m_size), m_bars.size());
		if (m_currentBar && m_currentBarEnd > time) {
			Assert(!m_bars.empty());
			AssertNe(pt::not_a_date_time, m_currentBarEnd);
			m_currentBar->highPrice = std::max(m_currentBar->highPrice, price);
			m_currentBar->lowPrice = std::min(m_currentBar->lowPrice, price);
			m_currentBar->closePrice = price;
			m_currentBar->volume += qty;
			return false;
		} else {
			LogCurrentBar();
			m_currentBarEnd = GetBarEnd(time);
			m_currentBar = &m_bars[size_t(m_size)];
			m_currentBar->time = time;
			m_currentBar->openPrice
				= m_currentBar->closePrice
				= m_currentBar->highPrice
				= m_currentBar->lowPrice
				= price;
			m_currentBar->volume = qty;
			Interlocking::Increment(m_size);
			AssertEq(size_t(m_size), m_bars.size());
			return true;
		}
	}

	template<typename Stat>
	boost::shared_ptr<typename Stat> CreateStat(size_t size) const {
		return boost::shared_ptr<typename Stat>(
			new Stat(
				m_service.shared_from_this(),
				size));
	}

};

//////////////////////////////////////////////////////////////////////////

BarService::BarService(
			const std::string &tag,
			boost::shared_ptr<Security> &security,
			const IniFileSectionRef &ini,
			const boost::shared_ptr<const Settings> &settings)
		: Service("BarService", tag, security, settings) {
	m_pimpl = new Implementation(*this, ini);
}

BarService::~BarService() {
	delete m_pimpl;
}

boost::shared_ptr<BarService> BarService::shared_from_this() {
	return boost::static_pointer_cast<BarService>(Base::shared_from_this());
}

boost::shared_ptr<const BarService> BarService::shared_from_this() const {
	return boost::static_pointer_cast<const BarService>(Base::shared_from_this());
}

bool BarService::OnNewTrade(
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide) {
	return m_pimpl->OnNewTrade(time, price, qty);
}

pt::time_duration BarService::GetBarSize() const {
	return m_pimpl->GetBarSize();
}

void BarService::UpdateAlogImplSettings(
			const Trader::Lib::IniFileSectionRef &ini) {
	m_pimpl->ReopenLog(ini, true);
}

const BarService::Bar & BarService::GetBar(size_t index) const {
	if (IsEmpty()) {
		throw BarDoesNotExistError("BarService is empty");
	} else if (index >= GetSize()) {
		throw BarDoesNotExistError("Index is out of range of BarService");
	}
	const Lock lock(GetMutex());
	const auto pos = m_pimpl->m_bars.find(index);
	Assert(pos != m_pimpl->m_bars.end());
	return pos->second;
}

const BarService::Bar & BarService::GetBarByReversedIndex(size_t index) const {
	if (IsEmpty()) {
		throw BarDoesNotExistError("BarService is empty");
	} else if (index >= GetSize()) {
		throw BarDoesNotExistError("Index is out of range of BarService");
	}
	const Lock lock(GetMutex());
	const auto pos = m_pimpl->m_bars.find(m_pimpl->m_size - index - 1);
	Assert(pos != m_pimpl->m_bars.end());
	return pos->second;
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
			offsetof(BarService::Bar, BarService::Bar::openPrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetClosePriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::closePrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetHighPriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::highPrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::ScaledPriceStat> BarService::GetLowPriceStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			ScaledPriceStat,
			offsetof(BarService::Bar, BarService::Bar::lowPrice)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

boost::shared_ptr<BarService::QtyStat> BarService::GetVolumeStat(
			size_t numberOfBars)
		const {
	typedef StatAccumulator<
			QtyStat,
			offsetof(BarService::Bar, BarService::Bar::volume)>
		Stat;
	return m_pimpl->CreateStat<Stat>(numberOfBars);
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateBarService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return boost::shared_ptr<Trader::Service>(
			new BarService(tag, security, ini, settings));
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateBarService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return boost::shared_ptr<Trader::Service>(
			new BarService(tag, security, ini, settings));
	}
#endif

//////////////////////////////////////////////////////////////////////////
