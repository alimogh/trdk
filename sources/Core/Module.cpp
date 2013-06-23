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
#include "ModuleSecurityList.hpp"
#include "Service.hpp"
#include "Security.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

Module::Log::Log(const Module &module)
		: m_log(module.GetContext().GetLog()),
		m_format(boost::format("[%1%] %2%") % module) {
	//...//
}

Module::Log::~Log() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

Module::SecurityList::SecurityList() {
	//...//
}

Module::SecurityList::~SecurityList() {
	//...//
}

Module::SecurityList::Iterator::Iterator(Implementation *pimpl)
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}

Module::SecurityList::Iterator::Iterator(const Iterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

Module::SecurityList::Iterator::~Iterator() {
	delete m_pimpl;
}

Module::SecurityList::Iterator &
Module::SecurityList::Iterator::operator =(const Iterator &rhs) {
	Assert(this != &rhs);
	Iterator(rhs).Swap(*this);
	return *this;
}

void Module::SecurityList::Iterator::Swap(Iterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

Security & Module::SecurityList::Iterator::dereference() const {
	return *m_pimpl->iterator->second;
}

bool Module::SecurityList::Iterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::Iterator::equal(
			const ConstIterator &rhs)
		const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::Iterator::increment() {
	++m_pimpl->iterator;
}

void Module::SecurityList::Iterator::decrement() {
	--m_pimpl->iterator;
}

void Module::SecurityList::Iterator::advance(difference_type n) {
	std::advance(m_pimpl->iterator, n);
}

Module::SecurityList::ConstIterator::ConstIterator(Implementation *pimpl)
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}

Module::SecurityList::ConstIterator::ConstIterator(const Iterator &rhs)
		: m_pimpl(new Implementation(rhs.m_pimpl->iterator)) {
	//...//
}

Module::SecurityList::ConstIterator::ConstIterator(const ConstIterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

Module::SecurityList::ConstIterator::~ConstIterator() {
	delete m_pimpl;
}

Module::SecurityList::ConstIterator &
Module::SecurityList::ConstIterator::operator =(const ConstIterator &rhs) {
	Assert(this != &rhs);
	ConstIterator(rhs).Swap(*this);
	return *this;
}

void Module::SecurityList::ConstIterator::Swap(ConstIterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

const Security & Module::SecurityList::ConstIterator::dereference() const {
	return *m_pimpl->iterator->second;
}

bool Module::SecurityList::ConstIterator::equal(
			const ConstIterator &rhs)
		const {
	Assert(this != &rhs);
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::ConstIterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::increment() {
	++m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::decrement() {
	--m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::advance(difference_type n) {
	std::advance(m_pimpl->iterator, n);
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

void Module::OnServiceStart(const Service &) {
	//...//
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
