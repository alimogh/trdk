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


BarService::BarService(
			const std::string &tag,
			boost::shared_ptr<Trader::Security> &security,
			const Trader::Lib::IniFileSectionRef &ini,
			const boost::shared_ptr<const Settings> &settings)
		: Service(tag, security),
		m_revision(nRevision),
		m_size(0),
		m_currentBar(nullptr),
		m_log(nullptr) {

	const std::string sizeStr = ini.ReadKey("size", false);
	const boost::regex expr(
		"(\\d+)\\s+([a-z]+)",
		boost::regex_constants::icase);
	boost::smatch what;
	if (!boost::regex_match(sizeStr , what, expr)) {
		Log::Error(
			"%1%: Wrong size size format: \"%2%\"."
				" Expected example: \"5 minutes\".",
			*this,
			what.str(0));
		throw Error("Wrong bar size settings");
	}

	m_barSize = boost::lexical_cast<decltype(m_barSize)>(what.str(1));
	if (boost::iequals(what.str(2), "seconds")) {
		if (60 % m_barSize) {
			Log::Error(
				"%1%: Wrong size specified: \"%2%\". 1 minute must be"
					" a multiple specified size.",
				*this,
				what.str(0));
			throw Error("Wrong bar size settings");
		}
		m_units = UNITS_SECONDS;
	} else if (boost::iequals(what.str(2), "minutes")) {
		if (60 % m_barSize) {
			Log::Error(
				"%1%: Wrong size specified: \"%2%\". 1 hour must be"
					" a multiple specified size.",
				*this,
				what.str(0));
			throw Error("Wrong bar size settings");
		}
		m_units = UNITS_MINUTES;
	} else if (boost::iequals(what.str(2), "hours")) {
		size_t tradeSessionMinuteCount
			=	(settings->GetCurrentTradeSessionEndime()
					- settings->GetCurrentTradeSessionStartTime()).minutes();
		if (tradeSessionMinuteCount % 60) {
			tradeSessionMinuteCount = ((tradeSessionMinuteCount / 60) + 1) * 60;
		}
		if (tradeSessionMinuteCount / 60 % m_barSize) {
			Log::Error(
				"%1%: Wrong size specified: \"%2%\". Trade session period must be"
					" a multiple specified size.",
				*this,
				what.str(0));
			throw Error("Wrong bar size settings");
		}
		m_units = UNITS_HOURS;
		throw Error("Hours units doesn't yet implemented");
	} else if (boost::iequals(what.str(2), "days")) {
		m_units = UNITS_DAYS;
		throw Error("Days units doesn't yet implemented");
	} else {
		Log::Error(
			"%1%: Wrong size specified: \"%2%\". Unknown units."
				"Supported: seconds, minutes, hours and days.",
			*this,
			what.str(0));
		throw Error("Wrong bar size settings");
	}

	{
		const std::string logType = ini.ReadKey("log", false);
		if (boost::iequals(logType, "none")) {
			//...//
		} else if (boost::iequals(logType, "csv")) {
			const fs::path path = Util::SymbolToFilePath(
				Defaults::GetBarsDataLogDir(),
				(boost::format("%1%_%2%_%3%")
						% what.str(2)
						% what.str(1)
						% security->GetFullSymbol())
					.str(),
				"csv");
			const bool isNew = !fs::exists(path);
			if (isNew) {
				fs::create_directories(path.branch_path());
			}
			m_log.reset(
				new std::ofstream(
					path.string(),
					std::ios::out | std::ios::ate | std::ios::app));
			if (!*m_log) {
				Log::Error(
					"%1%: Failed to open log file %2%",
					*this,
					path);
				throw Error("Failed to open log file");
			}
			if (isNew) {
				(*m_log)
					<< "<DATE>\t<TIME>\t<OPEN>\t<HIGH>\t<LOW>\t<CLOSE>\t<VOL>"
					<< std::endl;
			}
			(*m_log) << std::setfill('0');
			Log::Info("%1%: Logging bars into %2%.", path);
		} else {
			Log::Error(
				"%1%: Wrong log type settings: \"%2%\". Unknown type."
					" Supported: none and CSV.",
				*this,
				logType);
			throw Error("Wrong bars log type");
		}
	}

	Log::Info(
		"%1%: stated for \"%1%\" with size %2%. Log type: %3%.",
		*this,
		m_barSize,
		m_log ? "CSV" : "none");

}

void BarService::OnNewTrade(
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide) {
	AssertLe(size_t(m_size), m_bars.size());
	if (m_currentBar && m_currentBarEnd > time) {
		Assert(!m_bars.empty());
		AssertNe(pt::not_a_date_time, m_currentBarEnd);
		m_currentBar->high = std::max(m_currentBar->high, price);
		m_currentBar->low = std::min(m_currentBar->low, price);
		m_currentBar->closePrice = price;
		m_currentBar->volume += qty;
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
		Interlocking::Increment(m_revision);
	}
}

void BarService::LogCurrentBar() const {
	if (!m_log || !m_currentBar) {
		return;
	}
	AssertNe(pt::not_a_date_time, m_currentBarEnd);
	const auto barStartTime = m_currentBarEnd - GetBarSize();
	{
		const auto date = barStartTime.date();
		(*m_log)
			<< date.year()
			<< std::setw(2) << date.month().as_number()
			<< std::setw(2) << date.day();
	}
	{
		const auto time = barStartTime.time_of_day();
		(*m_log)
			<< "\t"
			<< std::setw(2) << time.hours()
			<< std::setw(2) << time.minutes()
			<< std::setw(2) << time.seconds();
	}
	(*m_log)
		<< "\t" << m_currentBar->openPrice
		<< "\t" << m_currentBar->high
		<< "\t" << m_currentBar->low
		<< "\t" << m_currentBar->closePrice
		<< "\t" << m_currentBar->volume
		<< std::endl;
}

pt::ptime BarService::GetBarEnd(
			const boost::posix_time::ptime &tradeTime)
		const {
	AssertNe(pt::not_a_date_time, tradeTime);
	static_assert(numberOfOrderSides, "Units list changed.");
	switch (m_units) {
		case UNITS_SECONDS:
			return
				tradeTime
				- pt::seconds(tradeTime.time_of_day().seconds())
				+ pt::seconds(m_barSize);
		case UNITS_MINUTES:
			return
				pt::ptime(tradeTime.date())
				+ pt::hours(tradeTime.time_of_day().hours())
				+ pt::minutes(
					((tradeTime.time_of_day().minutes() / m_barSize) + 1)
					* m_barSize);
		case UNITS_HOURS:
			throw Error("Days units doesn't yet implemented");
		case UNITS_DAYS:
			throw Error("Days units doesn't yet implemented");
		default:
			AssertFail("Unknown units type");
			throw Exception("Unknown bar service units type");
	}
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
