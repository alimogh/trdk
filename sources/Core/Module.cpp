/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"
#include "Service.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

Module::Log::Log(const Module &module)
		: m_format(boost::format("[%1%] %2%") % module),
		m_tag(module.GetTag()) {
	//...//
}

Module::Log::~Log() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class Module::Implementation : private boost::noncopyable {

public:

	Mutex m_mutex;
	
	const std::string m_typeName;
	const std::string m_name;
	const std::string m_tag;
	
	boost::shared_ptr<const Settings> m_settings;

	boost::scoped_ptr<Log> m_log;

	explicit Implementation(
				const std::string &typeName,
				const std::string &name,
				const std::string &tag,
				boost::shared_ptr<const Settings> &settings)
			: m_typeName(typeName),
			m_name(name),
			m_tag(tag),
			m_settings(settings) {
		//...//
	}

};

Module::Module(
			const std::string &typeName,
			const std::string &name,
			const std::string &tag,
			boost::shared_ptr<const Settings> settings)
		: m_pimpl(new Implementation(typeName, name, tag, settings)) {
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

const Settings & Module::GetSettings() const {
	return *m_pimpl->m_settings;
}

Module::Log & Module::GetLog() const throw() {
	return *m_pimpl->m_log;
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
