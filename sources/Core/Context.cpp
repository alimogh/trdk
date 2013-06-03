/**************************************************************************
 *   Created: 2013/01/31 12:14:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Context.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

Context::Exception::Exception(const char *what) throw()
		: Lib::Exception(what) {
	//...//
}

Context::UnknownSecurity::UnknownSecurity() throw()
		: Exception("Unknown security") {
	//...//
}

//////////////////////////////////////////////////////////////////////////

Context::Log::Log(const Context &) {
	//...//
}

Context::Log::~Log() {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class Context::Implementation : private boost::noncopyable {

public:

	Context::Log m_log;
	
	explicit Implementation(Context &context)
			: m_log(context) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Context::Context() {
	m_pimpl = new Implementation(*this);
}

Context::~Context() {
	delete m_pimpl;
}

Context::Log & Context::GetLog() const throw() {
	return m_pimpl->m_log;
}

Security & Context::GetSecurity(const Symbol &symbol) {
	auto result = FindSecurity(symbol);
	if (!result) {
		throw UnknownSecurity();
	}
	return *result;
}

const Security & Context::GetSecurity(const Symbol &symbol) const {
	auto result = FindSecurity(symbol);
	if (!result) {
		throw UnknownSecurity();
	}
	return *result;	
}

//////////////////////////////////////////////////////////////////////////
