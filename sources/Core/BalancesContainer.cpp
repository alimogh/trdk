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
#include "DropCopy.hpp"
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

struct Balance {
  Volume available;
  Volume locked;
};

typedef boost::unordered_map<std::string, Balance> Storage;
}

class BalancesContainer::Implementation : private boost::noncopyable {
 public:
  const TradingSystem &m_tradingSystem;
  ModuleEventsLog &m_eventsLog;
  ModuleTradingLog &m_tradingLog;
  Mutex m_mutex;
  Storage m_storage;

  explicit Implementation(const TradingSystem &tradingSystem,
                          ModuleEventsLog &eventsLog,
                          ModuleTradingLog &tradingLog)
      : m_tradingSystem(tradingSystem),
        m_eventsLog(eventsLog),
        m_tradingLog(tradingLog) {}

  void Set(const std::string &symbol,
           boost::optional<Volume> &&available,
           boost::optional<Volume> &&locked) {
    const WriteLock lock(m_mutex);
    const auto &it = m_storage.find(symbol);
    it != m_storage.cend()
        ? Update(*it, std::move(available), std::move(locked))
        : Insert(symbol, std::move(available), std::move(locked));
  }

  void Insert(const std::string &symbol,
              boost::optional<Volume> &&availableSource,
              boost::optional<Volume> &&lockedSource) {
    auto available = availableSource ? std::move(*availableSource) : 0;
    auto locked = lockedSource ? std::move(*lockedSource) : 0;

    const auto &result = m_storage.emplace(
        symbol, Balance{std::move(available), std::move(locked)});
    Assert(result.second);
    auto &storage = result.first->second;
    m_tradingLog.Write(
        "{'balance': {'symbol': '%1%', 'available': {'prev': null, 'new': "
        "%2$.8f, 'delta': %2$.8f}, 'locked': {'prev': null, 'new': %3$.8f, "
        "'delta': %3$.8f}}}",
        [&](TradingRecord &record) {
          record % result.first->first  // 1
              % storage.available       // 2
              % storage.locked;         // 3
        });

    Copy(symbol, storage);
  }

  void Update(Storage::value_type &storage,
              boost::optional<Volume> &&availableSource,
              boost::optional<Volume> &&lockedSource) {
    auto available = availableSource ? std::move(*availableSource)
                                     : storage.second.available;
    auto locked =
        lockedSource ? std::move(*lockedSource) : storage.second.locked;

    const Double availableDelta = available - storage.second.available;
    const Double lockedDelta = locked - storage.second.locked;
    if (availableDelta == 0 && lockedDelta == 0) {
      return;
    }

    m_tradingLog.Write(
        "{'balance': {'symbol': '%1%', 'available': {'prev': %2$.8f, 'new': "
        "%3$.8f, 'delta': %4$.8f}, 'locked': {'prev': %5$.8f, 'new': %6$.8f, "
        "'delta': %7$.8f}}}",
        [&](TradingRecord &record) {
          record % storage.first          // 1
              % storage.second.available  // 2
              % available                 // 3
              % availableDelta            // 4
              % storage.second.locked     // 5
              % locked                    // 6
              % lockedDelta;              // 7
        });
    storage.second.available = std::move(available);
    storage.second.locked = std::move(locked);

    Copy(storage.first, storage.second);
  }

  void Copy(const std::string &symbol, const Balance &balance) const {
    m_tradingSystem.GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
      dropCopy.CopyBalance(m_tradingSystem, symbol, balance.available,
                           balance.locked);
    });
  }
};

BalancesContainer::BalancesContainer(const TradingSystem &tradingSystem,
                                     ModuleEventsLog &eventsLog,
                                     ModuleTradingLog &tradingLog)
    : m_pimpl(boost::make_unique<Implementation>(
          tradingSystem, eventsLog, tradingLog)) {}

BalancesContainer::~BalancesContainer() = default;

Volume BalancesContainer::FindAvailableToTrade(
    const std::string &symbol) const {
  const ReadLock lock(m_pimpl->m_mutex);
  const auto &it = m_pimpl->m_storage.find(symbol);
  if (it == m_pimpl->m_storage.cend()) {
    return 0;
  }
  return it->second.available;
}

void BalancesContainer::Set(const std::string &symbol,
                            Volume &&available,
                            Volume &&locked) {
  m_pimpl->Set(symbol, std::move(available), std::move(locked));
}

void BalancesContainer::SetAvailableToTrade(const std::string &symbol,
                                            Volume &&volume) {
  m_pimpl->Set(symbol, std::move(volume), boost::none);
}

void BalancesContainer::SetLocked(const std::string &symbol, Volume &&volume) {
  m_pimpl->Set(symbol, boost::none, std::move(volume));
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
      auto &storage = it->second;

      const auto newAvailable = storage.available - delta;
      const auto newLocked = storage.locked + delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'available': {'prev': %2$.8f, 'new': "
          "%3$.8f, 'delta': %4$.8f}, 'locked': {'prev': %5$.8f, 'new': %6$.8f, "
          "'delta': %7$.8f}}}",
          [&](TradingRecord &record) {
            record % symbol          // 1
                % storage.available  // 2
                % newAvailable       // 3
                % -delta             // 4
                % storage.locked     // 5
                % newLocked          // 6
                % delta;             // 7
          });
      storage.available = newAvailable;
      storage.locked = newAvailable;

      m_pimpl->Copy(symbol, storage);

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
      auto &storage = it->second;

      const auto newAvailable = storage.available - delta;
      const auto newLocked = storage.locked + delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'available': {'prev': %2$.8f, 'new': "
          "%3$.8f, 'delta': %4$.8f}, 'locked': {'prev': %5$.8f, 'new': %6$.8f, "
          "'delta': %7$.8f}}}",
          [&](TradingRecord &record) {
            record % symbol          // 1
                % storage.available  // 2
                % newAvailable       // 3
                % -delta             // 4
                % storage.locked     // 5
                % newLocked          // 6
                % delta;             // 7
          });
      storage.available = newAvailable;
      storage.locked = newLocked;

      m_pimpl->Copy(symbol, storage);

      break;
    }
  }
}

void BalancesContainer::ForEach(
    const boost::function<void(const std::string &symbol,
                               const trdk::Volume &available,
                               const trdk::Volume &locked)> &callback) const {
  const ReadLock lock(m_pimpl->m_mutex);
  for (const auto &balance : m_pimpl->m_storage) {
    callback(balance.first, balance.second.available, balance.second.locked);
  }
}
