/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"
#include "Service.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

Module::Log::Log(const Module &module)
		: m_log(module.GetContext().GetLog()),
		m_format(boost::format("[%1%] %2%") % module) {
	//...//
}

Module::Log::~Log() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class Module::Implementation : private boost::noncopyable {

public:

	Mutex m_mutex;

	Context &m_context;
	
	const std::string m_typeName;
	const std::string m_name;
	const std::string m_tag;

	boost::scoped_ptr<Log> m_log;

	explicit Implementation(
				Context &context,
				const std::string &typeName,
				const std::string &name,
				const std::string &tag)
			: m_context(context),
			m_typeName(typeName),
			m_name(name),
			m_tag(tag) {
		//...//
	}

};

Module::Module(
			Context &context,
			const std::string &typeName,
			const std::string &name,
			const std::string &tag)
		: m_pimpl(new Implementation(context, typeName, name, tag)) {
	m_pimpl->m_log.reset(new Log(*this));
}

Module::~Module() {
	delete m_pimpl;
}

Module::Mutex & Module::GetMutex() const {
	return m_pimpl->m_mutex;
}

const std::string & Module::GetTypeName() const throw() {
	return m_pimpl->m_typeName;
}

const std::string & Module::GetName() const throw() {
	return m_pimpl->m_name;
}

const std::string & Module::GetTag() const throw() {
	return m_pimpl->m_tag;
}

void Module::OnServiceStart(const Service &) {
	//...//
}

Context & Module::GetContext() {
	return m_pimpl->m_context;
}

const Context & Module::GetContext() const {
	return const_cast<Module *>(this)->GetContext();
}

Module::Log & Module::GetLog() const throw() {
	return *m_pimpl->m_log;
}

void Module::UpdateSettings(const IniFileSectionRef &ini) {
	const Lock lock(GetMutex());
	UpdateAlogImplSettings(ini);
}

namespace {
	
	typedef boost::mutex SettingsReportMutex;
	typedef SettingsReportMutex::scoped_lock SettingsReportLock;
	static SettingsReportMutex mutex;

	typedef std::map<
			std::string,
			std::list<std::pair<std::string, std::string>>>
		SettingsReportCache; 
	static SettingsReportCache settingsReportcache;

}
void Module::ReportSettings(
			const SettingsReport::Report &settings)
		const {

	const SettingsReportLock lock(mutex);

	const SettingsReportCache::const_iterator cacheIt
		= settingsReportcache.find(GetTag());
	if (cacheIt != settingsReportcache.end() && cacheIt->second == settings) {
		return;
	}

	static fs::path path;
	if (path.string().empty()) {
		path = Defaults::GetLogFilePath();
		path /= "configuration.log";
		fs::create_directories(path.branch_path());
	}

	{
		std::ofstream f(
			path.c_str(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!f) {
			GetLog().Error("Failed to open log file %1%.", path);
			throw Exception("Failed to open log file");
		}
		f
			<< (boost::get_system_time() + GetEdtDiff())
			<< ' ' << GetName() << ':' << std::endl;
		foreach (const auto &s, settings) {
			f << "\t" << s.first << " = " << s.second << std::endl;
		}
		f << std::endl;
	}

	settingsReportcache[GetTag()] = settings;

}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(
			std::ostream &oss,
			const Module &module)
		throw() {
	try {
		oss
			<< module.GetTypeName()
			<< '.' << module.GetName()
			<< '.' << module.GetTag();
	} catch (...) {
		AssertFailNoException();
	}
	return oss;
}

//////////////////////////////////////////////////////////////////////////
