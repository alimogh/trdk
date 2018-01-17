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
#include "TradingLog.hpp"
#include "TradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace {
typedef boost::shared_mutex Mutex;
typedef boost::shared_lock<Mutex> ReadLock;
typedef boost::unique_lock<Mutex> WriteLock;
}

class BalancesContainer::Implementation : private boost::noncopyable {
 public:
  ModuleEventsLog &m_eventsLog;
  ModuleTradingLog &m_tradingLog;
  Mutex m_mutex;
  boost::unordered_map<std::string, Volume> m_storage;

  explicit Implementation(ModuleEventsLog &eventsLog,
                          ModuleTradingLog &tradingLog)
      : m_eventsLog(eventsLog), m_tradingLog(tradingLog) {}
};

BalancesContainer::BalancesContainer(ModuleEventsLog &eventsLog,
                                     ModuleTradingLog &tradingLog)
    : m_pimpl(boost::make_unique<Implementation>(eventsLog, tradingLog)) {}

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
      const Double delta = balance - it->second;
      if (delta == 0) {
        return;
      }
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'prev': %2$.8f, 'new': %3$.8f, "
          "'delta': %4$.8f}}",
          [&](TradingRecord &record) {
            record % symbol   // 1
                % it->second  // 2
                % balance     // 3
                % delta;      // 4
          });
      it->second = std::move(balance);
      return;
    }
  }
  {
    const auto &it =
        m_pimpl->m_storage.emplace(std::move(symbol), std::move(balance));
    Assert(it.second);
    if (it.first->second) {
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'prev': null, 'new': %2$.8f, 'delta': "
          "%2$.8f}}",
          [&](TradingRecord &record) {
            record % it.first->first  // 1
                % it.first->second;   // 2
          });
    }
  }
}

void BalancesContainer::ReduceAvailableToTradeByOrder(
    const Security &security,
    const Qty &qty,
    const Price &price,
    const OrderSide &side,
    const TradingSystem &tradingSystem) {
  AssertEq(SECURITY_TYPE_CRYPTO, security.GetSymbol().GetSecurityType());
  if (security.GetSymbol().GetSecurityType() != SECURITY_TYPE_CRYPTO) {
    return;
  }

  switch (side) {
    case ORDER_SIDE_SELL: {
      const auto &symbol = security.GetSymbol().GetBaseSymbol();
      const auto &delta = qty;

      const WriteLock lock(m_pimpl->m_mutex);
      const auto &it = m_pimpl->m_storage.find(symbol);
      if (it == m_pimpl->m_storage.cend()) {
        break;
      }

      const auto newBalance = it->second - delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'prev': %2$.8f, 'new': %3$.8f, "
          "'delta': %4$.8f}}",
          [&](TradingRecord &record) {
            record % symbol   // 1
                % it->second  // 2
                % newBalance  // 3
                % -delta;     // 4
          });
      it->second = newBalance;

      break;
    }

    case ORDER_SIDE_BUY: {
      const auto &symbol = security.GetSymbol().GetQuoteSymbol();
      auto delta = qty * price;
      delta += tradingSystem.CalcCommission(delta, security);

      const WriteLock lock(m_pimpl->m_mutex);
      const auto &it = m_pimpl->m_storage.find(symbol);
      if (it == m_pimpl->m_storage.cend()) {
        break;
      }

      const auto newBalance = it->second - delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'prev': %2$.8f, 'new': %3$.8f, "
          "'delta': %4$.8f}}",
          [&](TradingRecord &record) {
            record % symbol   // 1
                % it->second  // 2
                % newBalance  // 3
                % -delta;     // 4
          });
      it->second = newBalance;

      break;
    }
  }
}

void BalancesContainer::SetLocked(const std::string &&, const Volume &&) {}
