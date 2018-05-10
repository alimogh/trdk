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
#include "Context.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

namespace {

template <Lib::Concurrency::Profile profile>
struct ConcurrencyPolicyT {
  static_assert(profile == Lib::Concurrency::PROFILE_RELAX,
                "Wrong concurrency profile");
  typedef boost::shared_mutex Mutex;
  typedef boost::shared_lock<Mutex> ReadLock;
  typedef boost::unique_lock<Mutex> WriteLock;
};

template <>
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
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

std::string FormatStringId(const std::string &instanceName) {
  std::string result("MarketDataSource");
  if (!instanceName.empty()) {
    result += '.';
    result += instanceName;
  }
  return result;
}
}  // namespace

class MarketDataSource::Implementation : private boost::noncopyable {
 public:
  MarketDataSource &m_self;

  size_t m_index;

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

  struct AsyncTask {
    boost::mutex mutex;
    std::vector<boost::unique_future<void>> list;
  } m_asyncTasks;

 public:
  explicit Implementation(MarketDataSource &self,
                          Context &context,
                          const std::string &instanceName)
      : m_self(self),
        m_index(std::numeric_limits<size_t>::max()),
        m_context(context),
        m_instanceName(instanceName),
        m_stringId(FormatStringId(m_instanceName)),
        m_log(m_stringId, m_context.GetLog()),
        m_tradingLog(m_instanceName, m_context.GetTradingLog()) {}

  template <typename Securities, typename Callback>
  static void ForEachSecurity(const Securities &securities,
                              const Callback &callback) {
    const SecuritiesWriteLock lock(securities.mutex);
    for (const auto &security : securities.list) {
      callback(*security.second);
    }
  }

  ContractExpiration ResolveContractExpiration(const Symbol &symbol) const {
    const auto &date = m_context.GetCurrentTime().date() + gr::date_duration(1);
    if (m_context.HasExpirationCalendar()) {
      const auto &result = m_context.GetExpirationCalendar().Find(symbol, date);
      if (result) {
        return *result;
      }
    } else {
      const auto &result = m_self.FindContractExpiration(symbol, date);
      if (result) {
        return *result;
      }
    }
    boost::format error("Failed to find expiration info for \"%1%\"");
    error % symbol;
    throw Exception(error.str().c_str());
  }

  ContractExpiration ResolveNextContractExpiration(
      const Security &security) const {
    if (m_context.HasExpirationCalendar()) {
      auto it = m_context.GetExpirationCalendar().Find(
          security.GetSymbol(), security.GetExpiration().GetDate());
      Assert(it);
      AssertEq(security.GetExpiration().GetDate(), it->GetDate());
      if (it && ++it) {
        return *it;
      }
    } else {
      const auto &result = m_self.FindContractExpiration(
          security.GetSymbol(),
          security.GetExpiration().GetDate() + gr::days(1));
      if (result) {
        return std::move(*result);
      }
    }
    boost::format error("Failed to find next expiration info for \"%1%\" %2%");
    error % security % security.GetExpiration();
    throw Exception(error.str().c_str());
  }
};

//////////////////////////////////////////////////////////////////////////

MarketDataSource::MarketDataSource(Context &context,
                                   const std::string &instanceName)
    : m_pimpl(std::make_unique<Implementation>(*this, context, instanceName)) {}

MarketDataSource::MarketDataSource(MarketDataSource &&) = default;
MarketDataSource &MarketDataSource::operator=(MarketDataSource &&) = default;

MarketDataSource::~MarketDataSource() {
  try {
    if (!m_pimpl->m_asyncTasks.list.empty()) {
      m_pimpl->m_log.Debug("Waiting for %1% asynchronous tasks...",
                           m_pimpl->m_asyncTasks.list.size());
      for (auto &task : m_pimpl->m_asyncTasks.list) {
        task.wait();
      }
      m_pimpl->m_log.Debug("All asynchronous tasks completed.");
    }
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void MarketDataSource::AssignIndex(size_t index) {
  AssertEq(std::numeric_limits<size_t>::max(), m_pimpl->m_index);
  AssertNe(std::numeric_limits<size_t>::max(), index);
  m_pimpl->m_index = index;
}

size_t MarketDataSource::GetIndex() const {
  AssertNe(std::numeric_limits<size_t>::max(), m_pimpl->m_index);
  return m_pimpl->m_index;
}

Context &MarketDataSource::GetContext() { return m_pimpl->m_context; }

const Context &MarketDataSource::GetContext() const {
  return const_cast<MarketDataSource *>(this)->GetContext();
}

MarketDataSource::Log &MarketDataSource::GetLog() const noexcept {
  return m_pimpl->m_log;
}

MarketDataSource::TradingLog &MarketDataSource::GetTradingLog() const noexcept {
  return m_pimpl->m_tradingLog;
}

const std::string &MarketDataSource::GetInstanceName() const {
  return m_pimpl->m_instanceName;
}

const std::string &MarketDataSource::GetStringId() const noexcept {
  return m_pimpl->m_stringId;
}

Security &MarketDataSource::GetSecurity(const Symbol &symbol) {
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
          securities.list.emplace(std::make_pair(symbol, &newSecurity)).second);
      result = &newSecurity;
    }
  }
  GetLog().Debug("Loaded security \"%1%\".", *result);
  return *result;
}

Security &MarketDataSource::GetSecurity(const Symbol &symbol,
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
        securities.list.emplace(std::make_pair(std::move(key), result)).second);
  }
  GetLog().Debug("Loaded security \"%1%\" (%2%).", *result, expiration);
  return *result;
}

Security &MarketDataSource::CreateSecurity(const Symbol &symbol) {
  auto &result = CreateNewSecurityObject(symbol);
  Assert(this == &result.GetSource());
  switch (result.GetSymbol().GetSecurityType()) {
    case SECURITY_TYPE_FUTURES:
      if (!result.GetSymbol().IsExplicit()) {
        result.SetExpiration(
            pt::not_a_date_time,
            m_pimpl->ResolveContractExpiration(result.GetSymbol()));
      }
      break;
  }
  return result;
}

Security *MarketDataSource::FindSecurity(const Symbol &symbol) {
  auto &securities = m_pimpl->m_securitiesWithoutExpiration;
  const SecuritiesWriteLock lock(securities.mutex);
  const auto &security = securities.list.find(symbol);
  return security != securities.list.cend() ? &*security->second : nullptr;
}

const Security *MarketDataSource::FindSecurity(const Symbol &symbol) const {
  return const_cast<MarketDataSource *>(this)->FindSecurity(symbol);
}

Security *MarketDataSource::FindSecurity(const Symbol &symbol,
                                         const ContractExpiration &expiration) {
  auto &securities = m_pimpl->m_securitiesWithExpiration;
  const SecuritiesWriteLock lock(securities.mutex);
  const auto &security =
      securities.list.find(std::make_pair(symbol, expiration));
  return security != securities.list.cend() ? &*security->second : nullptr;
}

const Security *MarketDataSource::FindSecurity(
    const Symbol &symbol, const ContractExpiration &expiration) const {
  return const_cast<MarketDataSource *>(this)->FindSecurity(symbol, expiration);
}

size_t MarketDataSource::GetActiveSecurityCount() const {
  return m_pimpl->m_securitiesWithoutExpiration.list.size() +
         m_pimpl->m_securitiesWithExpiration.list.size();
}

void MarketDataSource::ForEachSecurity(
    const boost::function<void(Security &)> &callback) {
  m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithoutExpiration, callback);
  m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithExpiration, callback);
}
void MarketDataSource::ForEachSecurity(
    const boost::function<void(const Security &)> &callback) const {
  m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithoutExpiration, callback);
  m_pimpl->ForEachSecurity(m_pimpl->m_securitiesWithExpiration, callback);
}

boost::optional<ContractExpiration> MarketDataSource::FindContractExpiration(
    const Symbol &, const gr::date &) const {
  throw MethodIsNotImplementedException(
      "Market data source doesn't support contract expiration");
}

void MarketDataSource::SwitchToContract(Security &,
                                        const ContractExpiration &&) const {
  throw MethodIsNotImplementedException(
      "Market data source doesn't support contract expiration");
}

void MarketDataSource::SwitchToNextContract(Security &security) const {
  const boost::mutex::scoped_lock lock(m_pimpl->m_asyncTasks.mutex);
  m_pimpl->m_asyncTasks.list.emplace_back(boost::async([this, &security]() {
    GetLog().Debug("Started asynchronous task of switching \"%1%\"...",
                   security);
    SwitchToContract(security,
                     m_pimpl->ResolveNextContractExpiration(security));
    GetLog().Debug("Asynchronous task for \"%1%\" is completed.", security);
  }));
}

////////////////////////////////////////////////////////////////////////////////

std::ostream &trdk::operator<<(std::ostream &oss,
                               const MarketDataSource &marketDataSource) {
  oss << marketDataSource.GetStringId();
  return oss;
}

////////////////////////////////////////////////////////////////////////////////
