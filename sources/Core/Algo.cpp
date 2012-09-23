/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Algo.hpp"
#include "Security.hpp"
#include "PositionReporter.hpp"
#include "Position.hpp"
#include "Settings.hpp"
#include "MarketDataSource.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using namespace Trader;

Algo::Algo(const std::string &tag, boost::shared_ptr<Security> security)
		: Module(tag),
		m_security(security),
		m_positionReporter(nullptr) {
	//...//
}

Algo::~Algo() {
	delete m_positionReporter;
}

Algo::Mutex & Algo::GetMutex() {
	return m_mutex;
}

void Algo::UpdateSettings(const IniFile &ini, const std::string &section) {
	const Lock lock(m_mutex);
	UpdateAlogImplSettings(ini, section);
}

Security::Qty Algo::CalcQty(Security::ScaledPrice price, Security::ScaledPrice volume) const {
	return std::max<Security::Qty>(1, Security::Qty(volume / price));
}

boost::shared_ptr<const Security> Algo::GetSecurity() const {
	return const_cast<Algo *>(this)->GetSecurity();
}

boost::shared_ptr<Security> Algo::GetSecurity() {
	return m_security;
}

PositionReporter & Algo::GetPositionReporter() {
	if (!m_positionReporter) {
		m_positionReporter = CreatePositionReporter().release();
	}
	return *m_positionReporter;
}

boost::posix_time::ptime Algo::GetLastDataTime() {
	return boost::posix_time::not_a_date_time;
}

void Algo::RequestHistory(
			const HistoryMarketDataSource &iqFeed,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime) {
	iqFeed.RequestHistory(GetSecurity(), fromTime, toTime);
}

bool Algo::IsValidPrice(const Settings &settings) const {
	return settings.IsValidPrice(*m_security);
}

void Algo::ReportSettings(const SettingsReport &settings) const {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	
	static Mutex mutex;

	const Lock lock(mutex);

	typedef std::map<std::string, std::list<std::pair<std::string, std::string>>> Cache; 
	static Cache cache;
	const Cache::const_iterator cacheIt = cache.find(GetTag());
	if (cacheIt != cache.end()) {
		if (cacheIt->second == settings) {
			return;
		}
	}

	static fs::path path;
	if (path.string().empty()) {
		path = Defaults::GetLogFilePath();
		path /= "configuration.log";
		boost::filesystem::create_directories(path.branch_path());
	}

	{
		std::ofstream f(path.c_str(), std::ios::out | std::ios::ate | std::ios::app);
		if (!f) {
			Log::Error("Failed to open log file %1%.", path);
			throw Exception("Failed to open log file");
		}
		f
			<< (boost::get_system_time() + Util::GetEdtDiff())
			<< ' ' << GetName() << ':' << std::endl;
		foreach (const auto &s, settings) {
			f << "\t" << s.first << " = " << s.second << std::endl;
		}
		f << std::endl;
	}

	cache[GetTag()] = settings;

}

pt::ptime Algo::GetCurrentTime() const {
	return !m_security->GetSettings().IsReplayMode()
		?	boost::get_system_time()
		:	m_security->GetLastMarketDataTime();
}
