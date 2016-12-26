/**************************************************************************
 *   Created: 2012/07/15 13:22:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "RiskControl.hpp"
#include "Context.hpp"
#include "Security.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	template<Lib::Concurrency::Profile profile>
	struct ConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;
	};

	template<>
	struct ConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		//! @todo TRDK-141
		typedef Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock ReadLock;
		typedef Mutex::ScopedLock WriteLock;
	};

	typedef ConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy;

	typedef ConcurrencyPolicy::Mutex SecuritiesMutex;
	typedef ConcurrencyPolicy::ReadLock SecuritiesReadLock;
	typedef ConcurrencyPolicy::WriteLock SecuritiesWriteLock;

}

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	std::string FormatStringId(const std::string &instanceName) {
		std::string	result("MarketDataSource");
		if (!instanceName.empty()) {
			result += '.';
			result += instanceName;
		}
		return result;
	}

}

class MarketDataSource::Implementation : private boost::noncopyable {

public:

	typedef boost::unordered_map<trdk::Lib::Symbol, Security *> Securities;

public:

	const size_t m_index;

	Context &m_context;

	const std::string m_instanceName;
	const std::string m_stringId;

	TradingSystem::Log m_log;
	TradingSystem::TradingLog m_tradingLog;

	SecuritiesMutex m_securitiesMutex;
	Securities m_securities;

public:

	explicit Implementation(
			size_t index,
			Context &context,
			const std::string &instanceName)
		: m_index(index)
		, m_context(context)
		, m_instanceName(instanceName)
		, m_stringId(FormatStringId(m_instanceName))
		, m_log(m_stringId, m_context.GetLog())
		, m_tradingLog(m_instanceName, m_context.GetTradingLog()) {
		//...//
	}

public:

	Security * FindSecurity(const Symbol &symbol) {
		const auto &result = m_securities.find(symbol);
		return result != m_securities.end()
			?	&*result->second
			:	nullptr;
	}

};

//////////////////////////////////////////////////////////////////////////

MarketDataSource::Error::Error(const char *what) throw()
		: Base::Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &instanceName)
	: m_pimpl(new Implementation(index, context, instanceName)) {
	//...//
}

MarketDataSource::~MarketDataSource() {
	delete m_pimpl;
}

size_t MarketDataSource::GetIndex() const {
	return m_pimpl->m_index;
}

Context & MarketDataSource::GetContext() {
	return m_pimpl->m_context;
}

const Context & MarketDataSource::GetContext() const {
	return const_cast<MarketDataSource *>(this)->GetContext();
}

MarketDataSource::Log & MarketDataSource::GetLog() const throw() {
	return m_pimpl->m_log;
}

MarketDataSource::TradingLog & MarketDataSource::GetTradingLog() const throw() {
	return m_pimpl->m_tradingLog;
}

const std::string & MarketDataSource::GetInstanceName() const {
	return m_pimpl->m_instanceName;
}

const std::string & MarketDataSource::GetStringId() const throw() {
	return m_pimpl->m_stringId;
}

Security & MarketDataSource::GetSecurity(const Symbol &symbol) {
	trdk::Security *result = FindSecurity(symbol);
	if (result) {
		return *result;
	}
	{
		const SecuritiesWriteLock lock(m_pimpl->m_securitiesMutex);
		result = m_pimpl->FindSecurity(symbol);
		if (result) {
			return *result;
		}
		auto &newSecurity = CreateSecurity(symbol);
		m_pimpl->m_securities[symbol] = &newSecurity;
		result = &newSecurity;
	}
	GetLog().Debug("Loaded security \"%1%\".", *result);
	return *result;
}

Security & MarketDataSource::CreateSecurity(const Symbol &symbol) {
	auto &result = CreateNewSecurityObject(symbol);
	Assert(this == &result.GetSource());
	return result;
}

Security * MarketDataSource::FindSecurity(const Symbol &symbol) {
	const SecuritiesReadLock lock(m_pimpl->m_securitiesMutex);
	return m_pimpl->FindSecurity(symbol);
}

const Security * MarketDataSource::FindSecurity(const Symbol &symbol) const {
	return const_cast<MarketDataSource *>(this)->FindSecurity(symbol);
}

size_t MarketDataSource::GetActiveSecurityCount() const {
	return m_pimpl->m_securities.size();
}

void MarketDataSource::ForEachSecurity(
		const boost::function<bool (const Security &)> &pred)
		const {
	foreach (const auto &security, m_pimpl->m_securities) {
		if (!pred(*security.second)) {
			break;
		}
	}
}

void MarketDataSource::SwitchToNextContract(trdk::Security &security) {
	GetLog().Error(
		"Market data source does not support contact switching for %1%.",
		security);
	throw MethodDoesNotImplementedError(
		"Market data source does not support contact switching");
}

////////////////////////////////////////////////////////////////////////////////

std::ostream & trdk::operator <<(
		std::ostream &oss,
		const MarketDataSource &marketDataSource) {
	oss << marketDataSource.GetStringId();
	return oss;
}

////////////////////////////////////////////////////////////////////////////////

