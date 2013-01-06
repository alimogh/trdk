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

class Module::Implementation : private boost::noncopyable {

public:

	Mutex m_mutex;
	const std::string m_tag;
	boost::shared_ptr<const Settings> m_settings;

	explicit Implementation(
				const std::string &tag,
				boost::shared_ptr<const Settings> &settings)
			: m_tag(tag),
			m_settings(settings) {
		//...//
	}

};

Module::Module(
			const std::string &tag,
			boost::shared_ptr<const Settings> settings)
		: m_pimpl(new Implementation(tag, settings)) {
	//...//
}

Module::~Module() {
	delete m_pimpl;
}

Module::Mutex & Module::GetMutex() const {
	return m_pimpl->m_mutex;
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
