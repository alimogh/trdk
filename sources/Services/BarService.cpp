/**************************************************************************
 *   Created: 2012/11/15 23:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "BarService.hpp"
#include "Core/Settings.hpp"

using namespace Trader;
using namespace Trader::Services;
using namespace Trader::Lib;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

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

	std::unique_ptr<BarsLog> m_log;

public:

	explicit Implementation(BarService &service, const IniFileSectionRef &ini)
			: m_service(service),
			m_size(0),
			m_currentBar(nullptr),
			m_log(nullptr) {

		{
			const std::string sizeStr = ini.ReadKey("size", false);
			const boost::regex expr(
				"(\\d+)\\s+([a-z]+)",
				boost::regex_constants::icase);
			boost::smatch what;
			if (!boost::regex_match(sizeStr , what, expr)) {
				Log::Error(
					"%1%: Wrong size size format: \"%2%\". Example: \"5 minutes\".",
					m_service,
					what.str(0));
				throw Error("Wrong bar size settings");
			}
			m_barSizeStr = what.str(1);
			m_unitsStr = what.str(2);
		}

		m_barSize = boost::lexical_cast<decltype(m_barSize)>(m_barSizeStr);
		if (m_barSize <= 0) {
			Log::Error(
				"%1%: Wrong size specified: \"%2%\"."
					" Size can't be zero or less.",
				m_service,
				m_barSizeStr);
			throw Error("Wrong bar size settings");
		}

		if (boost::iequals(m_unitsStr, "seconds")) {
			if (60 % m_barSize) {
				Log::Error(
					"%1%: Wrong size specified: \"%2%\". 1 minute must be"
						" a multiple specified size.",
					m_service,
					m_unitsStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_SECONDS;
		} else if (boost::iequals(m_unitsStr, "minutes")) {
			if (60 % m_barSize) {
				Log::Error(
					"%1%: Wrong size specified: \"%2%\". 1 hour must be"
						" a multiple specified size.",
					m_service,
					m_unitsStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_MINUTES;
		} else if (boost::iequals(m_unitsStr, "hours")) {
			size_t tradeSessionMinuteCount
				=	(m_service.GetSettings().GetCurrentTradeSessionEndime()
							- m_service
								.GetSettings()
								.GetCurrentTradeSessionStartTime())
						.minutes();
			if (tradeSessionMinuteCount % 60) {
				tradeSessionMinuteCount
					= ((tradeSessionMinuteCount / 60) + 1) * 60;
			}
			if (tradeSessionMinuteCount / 60 % m_barSize) {
				Log::Error(
					"%1%: Wrong size specified: \"%2%\"."
						" Trade session period must be"
						" a multiple specified size.",
					m_service,
					m_unitsStr);
				throw Error("Wrong bar size settings");
			}
			m_units = UNITS_HOURS;
			throw Error("Hours units doesn't yet implemented");
		} else if (boost::iequals(m_unitsStr, "days")) {
			m_units = UNITS_DAYS;
			throw Error("Days units doesn't yet implemented");
		} else {
			Log::Error(
				"%1%: Wrong size specified: \"%2%\". Unknown units."
					"Supported: seconds, minutes, hours and days.",
				m_service,
				m_unitsStr);
			throw Error("Wrong bar size settings");
		}

		ReopenLog(ini, false);

		Log::Info(
			"%1%: stated for \"%1%\" with size %2%.",
			m_service,
			m_barSize);
	
	}

	void ReopenLog(const IniFileSectionRef &ini, bool isCritical) {

		const std::string logType = ini.ReadKey("log", false);
		if (boost::iequals(logType, "none")) {
			m_log.reset();
			return;
		} else if (!boost::iequals(logType, "csv")) {
			Log::Error(
				"%1%: Wrong log type settings: \"%2%\". Unknown type."
					" Supported: none and CSV.",
				m_service,
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
		if (m_log && m_log->path == log->path) {
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
			Log::Error(
				"%1%: Failed to open log file %2%",
				m_service,
				log->path);
			if (!isCritical) {
				throw Error("Failed to open log file");
			}
			return;
		}
		if (isNew) {
			log->file
				<< "<DATE>\t<TIME>\t<OPEN>\t<HIGH>\t<LOW>\t<CLOSE>\t<VOL>"
				<< std::endl;
		}
		log->file << std::setfill('0');

		Log::Info("%1%: Logging bars into %2%.", m_service, log->path);
		std::swap(log, m_log);

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
		if (!m_log || !m_currentBar) {
			return;
		}
		AssertNe(pt::not_a_date_time, m_currentBarEnd);
		const auto barStartTime = m_currentBarEnd - GetBarSize();
		{
			const auto date = barStartTime.date();
			m_log->file
				<< date.year()
				<< std::setw(2) << date.month().as_number()
				<< std::setw(2) << date.day();
		}
		{
			const auto time = barStartTime.time_of_day();
			m_log->file
				<< "\t"
				<< std::setw(2) << time.hours()
				<< std::setw(2) << time.minutes()
				<< std::setw(2) << time.seconds();
		}
		m_log->file
			<< "\t" << m_currentBar->openPrice
			<< "\t" << m_currentBar->high
			<< "\t" << m_currentBar->low
			<< "\t" << m_currentBar->closePrice
			<< "\t" << m_currentBar->volume
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
				//! @todo Implement hours bar service 
				throw Error("Hours units doesn't yet implemented");
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
			m_currentBar->high = std::max(m_currentBar->high, price);
			m_currentBar->low = std::min(m_currentBar->low, price);
			m_currentBar->closePrice = price;
			m_currentBar->volume += qty;
			return false;
		} else {
			LogCurrentBar();
			m_currentBarEnd = GetBarEnd(time);
			m_currentBar = &m_bars[size_t(m_size)];
			m_currentBar->openPrice
				= m_currentBar->closePrice
				= m_currentBar->high
				= m_currentBar->low
				= price;
			m_currentBar->volume = qty;
			Interlocking::Increment(m_size);
			AssertEq(size_t(m_size), m_bars.size());
			return true;
		}
	}

};

//////////////////////////////////////////////////////////////////////////

BarService::BarService(
			const std::string &tag,
			boost::shared_ptr<Security> &security,
			const IniFileSectionRef &ini,
			const boost::shared_ptr<const Settings> &settings)
		: Service(tag, security, settings) {
	m_pimpl = new Implementation(*this, ini);
}

BarService::~BarService() {
	delete m_pimpl;
}

const std::string & BarService::GetName() const {
	static const std::string name = "BarService";
	return name;
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
