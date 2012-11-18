/**************************************************************************
 *   Created: 2012/11/04 15:48:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityAlgo.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using namespace Trader;
using namespace Trader::Lib;

SecurityAlgo::SecurityAlgo(
			const std::string &tag,
			boost::shared_ptr<Security> security)
		: Module(tag),
		m_security(security) {
	//...//
}

SecurityAlgo::~SecurityAlgo() {
	//...//
}

void SecurityAlgo::UpdateSettings(const IniFileSectionRef &ini) {
	const Lock lock(GetMutex());
	UpdateAlogImplSettings(ini);
}

Qty SecurityAlgo::CalcQty(ScaledPrice price, ScaledPrice volume) const {
	return std::max<Qty>(1, Qty(volume / price));
}

boost::shared_ptr<const Security> SecurityAlgo::GetSecurity() const {
	return const_cast<SecurityAlgo *>(this)->GetSecurity();
}

boost::shared_ptr<Security> SecurityAlgo::GetSecurity() {
	return m_security;
}

bool SecurityAlgo::IsValidPrice(const Settings &settings) const {
	return settings.IsValidPrice(*m_security);
}

void SecurityAlgo::ReportSettings(
			const SettingsReport::Report &settings)
		const {

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;
	
	static Mutex mutex;

	const Lock lock(mutex);

	typedef std::map<
			std::string,
			std::list<std::pair<std::string, std::string>>>
		Cache; 
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
		std::ofstream f(
			path.c_str(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!f) {
			Log::Error("Failed to open log file %1%.", path);
			throw Trader::Lib::Exception("Failed to open log file");
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

pt::ptime SecurityAlgo::GetCurrentTime() const {
	return !m_security->GetSettings().IsReplayMode()
		?	boost::get_system_time()
		:	m_security->GetLastMarketDataTime();
}
