/*******************************************************************************
 *   Created: 2017/11/29 15:46:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BalancesContainer.hpp"
#include "EventsLog.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace {
typedef boost::shared_mutex Mutex;
typedef boost::shared_lock<Mutex> ReadLock;
typedef boost::unique_lock<Mutex> WriteLock;
}

class BalancesContainer::Implementation : private boost::noncopyable {
 public:
  ModuleEventsLog &m_log;
  Mutex m_mutex;
  boost::unordered_map<std::string, Volume> m_storage;

  explicit Implementation(ModuleEventsLog &log) : m_log(log) {}
};

BalancesContainer::BalancesContainer(ModuleEventsLog &log)
    : m_pimpl(boost::make_unique<Implementation>(log)) {}

BalancesContainer::~BalancesContainer() = default;

boost::optional<Volume> BalancesContainer::FindAvailableToTrade(
    const std::string &symbol) const {
  const ReadLock lock(m_pimpl->m_mutex);
  const auto &it = m_pimpl->m_storage.find(symbol);
  if (it == m_pimpl->m_storage.cend()) {
    return boost::none;
  }
  return it->second;
}

void BalancesContainer::SetAvailableToTrade(const std::string &&symbol,
                                            const Volume &&balance) {
  const WriteLock lock(m_pimpl->m_mutex);
  {
    const auto &it = m_pimpl->m_storage.find(symbol);
    if (it != m_pimpl->m_storage.cend()) {
      it->second = std::move(balance);
      return;
    }
  }
  {
    const auto &it =
        m_pimpl->m_storage.emplace(std::move(symbol), std::move(balance));
    Assert(it.second);
    if (it.first->second) {
      m_pimpl->m_log.Info("\"%1%\" balance: %2$.8f.",
                          it.first->first,    // 1
                          it.first->second);  // 2
    }
  }
}
