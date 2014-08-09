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
	Params m_params;
	
	explicit Implementation(Context &context)
			: m_log(context),
			m_params(context) {
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

Context::Params & Context::GetParams() {
	return m_pimpl->m_params;
}

const Context::Params & Context::GetParams() const {
	return const_cast<Context *>(this)->GetParams();
}

//////////////////////////////////////////////////////////////////////////

class Context::Params::Implementation : private boost::noncopyable {

public:

#	ifdef BOOST_WINDOWS
		typedef Concurrency::reader_writer_lock Mutex;
		typedef Mutex::scoped_lock_read ReadLock;
		typedef Mutex::scoped_lock WriteLock;
#	else
		//! @todo TRDK-144
		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;
#	endif

	typedef std::map<std::string, std::string> Storage;

	Implementation(const Context &context)
			: m_context(context),
			m_revision(0) {
		Assert(m_revision.is_lock_free());
	}

	const Context &m_context;
	boost::atomic<Revision> m_revision;
	Mutex m_mutex;
	Storage m_storage;

};

Context::Params::Exception::Exception(const char *what) throw()
		: Context::Exception(what) {
	//...//
}

Context::Params::Exception::~Exception() {
	//...//
}

Context::Params::KeyDoesntExistError::KeyDoesntExistError(
			const char *what)
		throw()
		: Exception(what) {
	//...//
}

Context::Params::KeyDoesntExistError::~KeyDoesntExistError() {
	//...//
}

Context::Params::Params(const Context &context)
		: m_pimpl(new Implementation(context)) {
	//...//
}

Context::Params::~Params() {
	delete m_pimpl;
}

std::string Context::Params::operator [](const std::string &key) const {
	const Implementation::ReadLock lock(m_pimpl->m_mutex);
	const auto &pos = m_pimpl->m_storage.find(key);
	if (pos == m_pimpl->m_storage.end()) {
		boost::format message("Context parameter \"%1%\" doesn't exist");
		message % key;
		throw KeyDoesntExistError(message.str().c_str());
	}
	return pos->second;
}

void Context::Params::Update(
			const std::string &key,
			const std::string &newValue) {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	++m_pimpl->m_revision;
	auto it = m_pimpl->m_storage.find(key);
	if (it != m_pimpl->m_storage.end()) {
		m_pimpl->m_context.GetLog().Debug(
			"Context param \"%1%\" update (%2%): \"%3%\" -> \"%4%\"...",
			boost::make_tuple(
				boost::cref(key),
				Revision(m_pimpl->m_revision),
				boost::cref(it->second),
				boost::cref(newValue)));
		it->second = newValue;
	} else {
		m_pimpl->m_context.GetLog().Debug(
			"Context param \"%1%\" create (%2%): \"%3%\"...",
			boost::make_tuple(
				boost::cref(key),
				Revision(m_pimpl->m_revision),
				boost::cref(newValue)));
		m_pimpl->m_storage.insert(std::make_pair(key, newValue));
	}
}

bool Context::Params::IsExist(const std::string &key) const {
	const Implementation::ReadLock lock(m_pimpl->m_mutex);
	return m_pimpl->m_storage.find(key) != m_pimpl->m_storage.end();
}

Context::Params::Revision Context::Params::GetRevision() const {
	return m_pimpl->m_revision;
}

////////////////////////////////////////////////////////////////////////////////
