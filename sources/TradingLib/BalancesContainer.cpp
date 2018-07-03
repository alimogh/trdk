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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace {
typedef boost::shared_mutex Mutex;
typedef boost::shared_lock<Mutex> ReadLock;
typedef boost::unique_lock<Mutex> WriteLock;

struct Balance {
  Volume available;
  Volume locked;
};

typedef boost::unordered_map<std::string, Balance> Storage;
}  // namespace

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
           boost::optional<Volume> available,
           boost::optional<Volume> locked) {
    const auto &resolvedSymbol =
        m_tradingSystem.GetContext().GetSettings().ResolveSymbolAlias(symbol);
    const WriteLock lock(m_mutex);
    const auto &it = m_storage.find(resolvedSymbol);
    it != m_storage.cend()
        ? Update(*it, std::move(available), std::move(locked))
        : Insert(resolvedSymbol, std::move(available), std::move(locked));
  }

  void Insert(const std::string &symbol,
              boost::optional<Volume> availableSource,
              boost::optional<Volume> lockedSource) {
    auto available = availableSource ? std::move(*availableSource) : 0;
    auto locked = lockedSource ? std::move(*lockedSource) : 0;

    if (available == 0 && locked == 0) {
      return;
    }

    const auto &result = m_storage.emplace(
        symbol, Balance{std::move(available), std::move(locked)});
    Assert(result.second);
    auto &storage = result.first->second;
    m_tradingLog.Write(
        "{'balance': {'symbol': '%1%', 'available': {'prev': null, 'new': "
        "%2%, 'delta': %2%}, 'locked': {'prev': null, 'new': %3%, 'delta': "
        "%3%}}}",
        [&](TradingRecord &record) {
          record % result.first->first  // 1
              % storage.available       // 2
              % storage.locked;         // 3
        });

    Copy(symbol, storage);
  }

  void Update(Storage::value_type &storage,
              boost::optional<Volume> availableSource,
              boost::optional<Volume> lockedSource) {
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
        "{'balance': {'symbol': '%1%', 'available': {'prev': %2%, 'new': "
        "%3%, 'delta': %4%}, 'locked': {'prev': %5%, 'new': %6%, 'delta': "
        "%7%}}}",
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

Volume BalancesContainer::GetAvailableToTrade(const std::string &symbol) const {
  const ReadLock lock(m_pimpl->m_mutex);
  const auto &it = m_pimpl->m_storage.find(symbol);
  if (it == m_pimpl->m_storage.cend()) {
    return 0;
  }
  return it->second.available;
}

void BalancesContainer::Set(const std::string &symbol,
                            Volume available,
                            Volume locked) {
  m_pimpl->Set(symbol, std::move(available), std::move(locked));
}

void BalancesContainer::SetAvailableToTrade(const std::string &symbol,
                                            Volume volume) {
  m_pimpl->Set(symbol, std::move(volume), boost::none);
}

void BalancesContainer::SetLocked(const std::string &symbol, Volume volume) {
  m_pimpl->Set(symbol, boost::none, std::move(volume));
}

void BalancesContainer::ReduceAvailableToTradeByOrder(
    const Security &security,
    const Qty &qty,
    const Price &price,
    const OrderSide &side,
    const TradingSystem &tradingSystem) {
  AssertEq(+SecurityType::Crypto, security.GetSymbol().GetSecurityType());
  if (security.GetSymbol().GetSecurityType() != +SecurityType::Crypto) {
    return;
  }

  switch (side) {
    case OrderSide::Sell: {
      const auto &symbol = security.GetSymbol().GetBaseSymbol();
      auto delta = qty;

      const WriteLock lock(m_pimpl->m_mutex);
      const auto &it = m_pimpl->m_storage.find(symbol);
      if (it == m_pimpl->m_storage.cend()) {
        m_pimpl->m_eventsLog.Warn(
            "Failed to reduce the balance to %1% \"%2%\" as there is no "
            "balance for symbol \"%3%\".",
            side,      // 1
            security,  // 2
            symbol);   // 3
        break;
      }
      auto &storage = it->second;

      if (storage.available < delta) {
        m_pimpl->m_eventsLog.Warn(
            "Failed to reduce the balance by %1% to %2% \"%3%\" as result for "
            "symbol \"%4%\" will be negative.",
            delta,     // 1
            side,      // 2
            security,  // 3
            symbol);   // 4
        delta = storage.available;
      }
      const auto newAvailable = storage.available - delta;
      const auto newLocked = storage.locked + delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'available': {'prev': %2%, 'new': "
          "%3%, 'delta': %4%}, 'locked': {'prev': %5%, 'new': %6%, 'delta': "
          "%7%}}}",
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

    case OrderSide::Buy: {
      const auto &symbol = security.GetSymbol().GetQuoteSymbol();
      auto delta = qty * price;
      delta += tradingSystem.CalcCommission(qty, price, side, security);

      const WriteLock lock(m_pimpl->m_mutex);
      const auto &it = m_pimpl->m_storage.find(symbol);
      if (it == m_pimpl->m_storage.cend()) {
        m_pimpl->m_eventsLog.Warn(
            "Failed to reduce the balance to %1% \"%2%\" as there is no "
            "balance for symbol \"%3%\".",
            side,      // 1
            security,  // 2
            symbol);   // 3
        break;
      }
      auto &storage = it->second;

      if (storage.available < delta) {
        m_pimpl->m_eventsLog.Warn(
            "Failed to reduce the balance by %1% to %2% \"%3%\" as result for "
            "symbol \"%4%\" will be negative.",
            delta,     // 1
            side,      // 2
            security,  // 3
            symbol);   // 4
        delta = storage.available;
      }
      const auto newAvailable = storage.available - delta;
      const auto newLocked = storage.locked + delta;
      m_pimpl->m_tradingLog.Write(
          "{'balance': {'symbol': '%1%', 'available': {'prev': %2%, 'new': "
          "%3%, 'delta': %4%}, 'locked': {'prev': %5%, 'new': %6%, 'delta': "
          "%7%}}}",
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
