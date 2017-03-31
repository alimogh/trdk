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
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

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

	const size_t m_index;

	Context &m_context;

	const std::string m_instanceName;
	const std::string m_stringId;

	TradingSystem::Log m_log;
	TradingSystem::TradingLog m_tradingLog;

	struct {
		mutable SecuritiesMutex mutex;
		boost::unordered_map<Symbol, Security *> list;
	} m_securitiesWithoutExpiration;

	struct {
		mutable SecuritiesMutex mutex;
		std::map<std::pair<Symbol, ContractExpiration>, Security *> list;
	} m_securitiesWithExpiration;

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

	template<typename Securities>
	static void ForEachSecurity(
			const Securities &securities,
			const boost::function<bool (const Security &)> &pred) {
		const SecuritiesWriteLock lock(securities.mutex);
		for (const auto &security: securities.list) {
			if (!pred(*security.second)) {
				break;
			}
		}
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
		auto &securities = m_pimpl->m_securitiesWithoutExpiration;
		const SecuritiesWriteLock lock(securities.mutex);
		{
			const auto &security = securities.list.find(symbol);
			if (security != securities.list.cend()) {
				return *security->second;
			}
		}
		{
			auto &newSecurity = CreateSecurity(symbol);
			Verify(
				securities.list.emplace(std::make_pair(symbol, &newSecurity))
					.second);
			result = &newSecurity;
		}
	}
	GetLog().Info("Loaded security \"%1%\".", *result);
	return *result;
}

Security & MarketDataSource::GetSecurity(
		const Symbol &symbol,
		const ContractExpiration &expiration) {
	trdk::Security *result = FindSecurity(symbol, expiration);
	if (result) {
		return *result;
	}
	{
		const auto key = std::make_pair(symbol, expiration);
		auto &securities = m_pimpl->m_securitiesWithExpiration;
		const SecuritiesWriteLock lock(securities.mutex);
		{
			const auto &security = securities.list.find(key);
			if (security != securities.list.cend()) {
				return *security->second;
			}
		}
		result = &CreateSecurity(symbol);
		result->SetExpiration(pt::not_a_date_time, expiration);
		Verify(
			securities.list.emplace(std::make_pair(std::move(key), result))
				.second);
	}
	GetLog().Info("Loaded security \"%1%\" (%2%).", *result, expiration);
	return *result;
}

Security & MarketDataSource::CreateSecurity(const Symbol &symbol) {
	auto &result = CreateNewSecurityObject(symbol);
	Assert(this == &result.GetSource());
	return result;
}

Security * MarketDataSource::FindSecurity(const Symbol &symbol) {
	auto &securities = m_pimpl->m_securitiesWithoutExpiration;
	const SecuritiesWriteLock lock(securities.mutex);
	const auto &security = securities.list.find(symbol);
	return security != securities.list.cend()
		? &*security->second
		: nullptr;
}

const Security * MarketDataSource::FindSecurity(const Symbol &symbol) const {
	return const_cast<MarketDataSource *>(this)->FindSecurity(symbol);
}

Security * MarketDataSource::FindSecurity(
		const Symbol &symbol,
		const ContractExpiration &expiration) {
	auto &securities = m_pimpl->m_securitiesWithExpiration;
	const SecuritiesWriteLock lock(securities.mutex);
	const auto &security
		= securities.list.find(std::make_pair(symbol, expiration));
	return security != securities.list.cend()
		? &*security->second
		: nullptr;
}

const Security * MarketDataSource::FindSecurity(
		const Symbol &symbol,
		const ContractExpiration &expiration)
		const {
	return const_cast<MarketDataSource *>(this)->FindSecurity(
		symbol,
		expiration);
}

size_t MarketDataSource::GetActiveSecurityCount() const {
	return
		m_pimpl->m_securitiesWithoutExpiration.list.size()
		+ m_pimpl->m_securitiesWithExpiration.list.size();
}

void MarketDataSource::ForEachSecurity(
		const boost::function<bool (const Security &)> &pred)
		const {
	m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithoutExpiration, pred);
	m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithExpiration, pred);
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

