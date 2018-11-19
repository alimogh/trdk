/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/RiskControl.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "ContextBootstrap.hpp"
#include "SubscriptionsManager.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace ptr = boost::property_tree;
namespace fs = boost::filesystem;
namespace ids = boost::uuids;

using namespace trdk;
using namespace Lib;
using namespace Engine;

class Engine::Context::Implementation {
 public:
  class State;

  Engine::Context &m_context;

  static_assert(numberOfTradingModes == 3, "List changed.");
  boost::array<std::unique_ptr<RiskControl>, numberOfTradingModes>
      m_riskControl;

  const fs::path m_fileLogsDir;

  std::unique_ptr<ExpirationCalendar> m_expirationCalendar;

  std::vector<DllObjectPtr<TradingSystem>> m_tradingSystems;
  std::vector<DllObjectPtr<MarketDataSource>> m_marketDataSources;

  bool m_isStopped;
  std::unique_ptr<State> m_state;

  explicit Implementation(Engine::Context &context)
      : m_context(context), m_isStopped(false) {
    static_assert(numberOfTradingModes == 3, "List changed.");
    for (size_t i = 0; i < m_riskControl.size(); ++i) {
      m_riskControl[i] = boost::make_unique<RiskControl>(
          m_context, context.GetSettings().GetConfig(),
          static_cast<TradingMode>(i));
    }

    {
      const auto &expiration =
          m_context.GetSettings().GetConfig().get_child_optional("expiration");
      if (expiration) {
        const auto &expirationCalendarFilePath =
            Normalize(expiration->get<fs::path>("calendarCsv"));
        m_context.GetLog().Debug("Loading expiration calendar from %1%...",
                                 expirationCalendarFilePath);
        auto expirationCalendar = std::make_unique<ExpirationCalendar>();
        expirationCalendar->ReloadCsv(expirationCalendarFilePath);
        const auto stat = expirationCalendar->CalcStat();
        const auto *const message =
            "Expiration calendar has %1% symbol(s) and %2% expiration(s).";
        stat.numberOfExpirations &&stat.numberOfSymbols
            ? m_context.GetLog().Info(message, stat.numberOfSymbols,
                                      stat.numberOfExpirations)
            : m_context.GetLog().Warn(message, stat.numberOfSymbols,
                                      stat.numberOfExpirations);
        m_expirationCalendar.swap(expirationCalendar);
      }
    }
    boost::tie(m_tradingSystems, m_marketDataSources) = LoadSources(m_context);
  }
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = delete;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;

  ~Implementation() { m_context.GetTradingLog().WaitForFlush(); }

  RiskControl &GetRiskControl(const TradingMode &mode) {
    return *m_riskControl[mode];
  }
};

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation::State {
 public:
  Context &m_context;
  SubscriptionsManager m_subscriptionsManager;
  std::vector<DllObjectPtr<Strategy>> m_strategies;
  DropCopy *dropCopy;

  explicit State(Context &context, DropCopy *dropCopy)
      : m_context(context),
        m_subscriptionsManager(context),
        dropCopy(dropCopy) {}
  State(State &&) = delete;
  State(const State &) = delete;
  State &operator=(State &&) = delete;
  State &operator=(const State &) = delete;
  ~State() {
    //     Assert(!m_subscriptionsManager.IsActive());
    // ... no new events expected, wait until old records will be flushed...
    m_context.GetTradingLog().WaitForFlush();
    // ... then we can destroy objects and unload DLLs...
  }

  void ReportState() const {
    {
      std::vector<std::string> markedDataSourcesStat;
      size_t commonSecuritiesCount = 0;
      m_context.ForEachMarketDataSource([&](const MarketDataSource &source) {
        std::ostringstream oss;
        oss << markedDataSourcesStat.size() + 1;
        if (!source.GetInstanceName().empty()) {
          oss << " (" << source.GetInstanceName() << ")";
        }
        oss << ": " << source.GetActiveSecurityCount();
        markedDataSourcesStat.push_back(oss.str());
        commonSecuritiesCount += source.GetActiveSecurityCount();
      });
      m_context.GetLog().Debug(
          "Loaded %1% market data sources with %2% securities: %3%.",
          markedDataSourcesStat.size(), commonSecuritiesCount,
          boost::join(markedDataSourcesStat, ", "));
    }

    m_context.GetLog().Debug("Loaded %1% strategy instances.",
                             m_strategies.size());
  }

  Strategy &GetStrategy(const ids::uuid &id) {
    for (auto &strategy : m_strategies) {
      if (strategy->GetId() == id) {
        return *strategy;
      }
    }
    throw Exception("Strategy with the given ID is not existent");
  }
};

//////////////////////////////////////////////////////////////////////////

Engine::Context::Context(Log &log, TradingLog &tradingLog, Settings &&settings)
    : Base(log, tradingLog, std::move(settings)),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

Engine::Context::~Context() {
  if (!m_pimpl->m_isStopped) {
    Stop(STOP_MODE_IMMEDIATELY);
  }
}

void Engine::Context::Start(
    const boost::function<void(const std::string &)> &startProgressCallback,
    const boost::function<bool(const std::string &)> &startErrorCallback,
    DropCopy *dropCopy) {
  GetLog().Debug("Starting...");

  if (m_pimpl->m_isStopped) {
    throw LogicError("Already is stopped");
  }
  if (m_pimpl->m_state) {
    throw LogicError("Already is started");
  }

  // It must be destroyed after state-object, as it state-object has
  // sub-objects from this DLL:
  m_pimpl->m_state.reset(new Implementation::State(*this, dropCopy));

  try {
    {
      std::set<size_t> errors;

      // Market data system should be connected before booting as sometimes
      // security objects can be created without market data source.
      for (size_t i = 0; i < m_pimpl->m_marketDataSources.size();) {
        auto &source = m_pimpl->m_marketDataSources[i];
        if (startProgressCallback) {
          startProgressCallback("Connecting to " + source->GetTitle() +
                                " market data source...");
        }
        try {
          source->Connect();
        } catch (const ConnectError &ex) {
          boost::format error(
              R"(Failed to connect to market data source "%1%": "%2%")");
          error % source->GetInstanceName()  // 1
              % ex;                          // 2
          if (!startErrorCallback) {
            throw ConnectError(error.str().c_str());
          }
          boost::format message(
              R"(Failed to connect to market data source %1%: %2%.)");
          message % source->GetTitle()  // 1
              % ex;                     // 2
          if (!startErrorCallback(message.str())) {
            continue;
          }
          GetLog().Warn("Ignoring error: \"%1%\"...", error);
          Verify(errors.emplace(i).second);
        } catch (const Lib::Exception &ex) {
          GetLog().Error("Failed to make market data connection: \"%1%\".", ex);
          throw Exception("Failed to make market data connection");
        }
        ++i;
      }

      for (size_t i = 0; i < m_pimpl->m_tradingSystems.size();) {
        if (errors.count(i)) {
          ++i;
          continue;
        }
        auto &tradingSystem = m_pimpl->m_tradingSystems[i];
        if (!tradingSystem) {
          ++i;
          continue;
        }
        if (startProgressCallback) {
          startProgressCallback("Connecting to " + tradingSystem->GetTitle() +
                                " trading system...");
        }
        try {
          tradingSystem->Connect();
        } catch (const ConnectError &ex) {
          boost::format error(
              R"(Failed to connect to trading system "%1%": "%2%")");
          error % tradingSystem->GetInstanceName()  // 1
              % ex;                                 // 2
          if (!startErrorCallback) {
            throw ConnectError(error.str().c_str());
          }
          boost::format message(
              R"(Failed to connect to trading system %1%: %2%)");
          message % tradingSystem->GetTitle()  // 1
              % ex;                            // 2
          GetLog().Debug(error.str().c_str());
          if (!startErrorCallback(message.str())) {
            continue;
          }
          GetLog().Warn("Ignoring error: \"%1%\"...", error);
          Verify(errors.emplace(i).second);
        } catch (const Lib::Exception &ex) {
          GetLog().Error("Failed to make trading system connection: \"%1%\".",
                         ex);
          throw Exception("Failed to make trading system connection");
        }
        ++i;
      }

      if (!errors.empty()) {
        AssertEq(m_pimpl->m_marketDataSources.size(),
                 m_pimpl->m_tradingSystems.size());
        {
          std::vector<DllObjectPtr<MarketDataSource>> marketDataSources;
          for (size_t i = 0; i < m_pimpl->m_marketDataSources.size(); ++i) {
            if (errors.count(i)) {
              continue;
            }
            marketDataSources.emplace_back(
                std::move(m_pimpl->m_marketDataSources[i]));
          }
          m_pimpl->m_marketDataSources = std::move(marketDataSources);
        }
        for (const auto &error : boost::adaptors::reverse(errors)) {
          m_pimpl->m_tradingSystems.erase(m_pimpl->m_tradingSystems.begin() +
                                          error);
        }
        AssertEq(m_pimpl->m_marketDataSources.size(),
                 m_pimpl->m_tradingSystems.size());

        m_pimpl->m_tradingSystems.shrink_to_fit();
        m_pimpl->m_marketDataSources.shrink_to_fit();
      }
    }

    if (startProgressCallback) {
      startProgressCallback("Bootstrapping modules...");
    }

    AssertEq(m_pimpl->m_marketDataSources.size(),
             m_pimpl->m_tradingSystems.size());
    for (size_t i = 0; i < m_pimpl->m_marketDataSources.size(); ++i) {
      m_pimpl->m_marketDataSources[i]->AssignIndex(i);
    }
    {
      size_t index = 0;
      for (auto &tradingSystem : m_pimpl->m_tradingSystems) {
        if (tradingSystem) {
          tradingSystem->AssignIndex(index);
        }
        ++index;
      }
    }

    try {
      m_pimpl->m_state->m_strategies =
          LoadStrategies(*this, m_pimpl->m_state->m_subscriptionsManager);
    } catch (const Lib::Exception &ex) {
      GetLog().Error(R"(Failed to init engine context: "%1%".)", ex);
      throw Exception("Failed to init engine context");
    }

    m_pimpl->m_state->ReportState();

    if (startProgressCallback) {
      startProgressCallback("Final initialization...");
    }

    ForEachMarketDataSource([&](MarketDataSource &source) -> bool {
      try {
        source.SubscribeToSecurities();
      } catch (const ConnectError &ex) {
        boost::format message(
            "Failed to make market data subscription: \"%1%\"");
        message % ex;
        throw ConnectError(message.str().c_str());
      } catch (const Lib::Exception &ex) {
        GetLog().Error("Failed to make market data subscription: \"%1%\".", ex);
        throw Exception("Failed to make market data subscription");
      }
      return true;
    });

    OnStarted();

    m_pimpl->m_state->m_subscriptionsManager.Activate();

    RaiseStateUpdate(Context::STATE_ENGINE_STARTED);
  } catch (...) {
    m_pimpl->m_state.reset();
    throw;
  }
}

void Engine::Context::Stop(const StopMode &stopMode) {
  if (m_pimpl->m_isStopped) {
    throw LogicError("Already is stopped");
  } else if (!m_pimpl->m_state) {
    return;
  }
  m_pimpl->m_isStopped = true;

  const auto *stopModeStr = "unknown";
  static_assert(numberOfStopModes == 3, "Stop mode list changed.");
  switch (stopMode) {
    case STOP_MODE_IMMEDIATELY:
      stopModeStr = "immediately";
      break;
    case STOP_MODE_GRACEFULLY_ORDERS:
      stopModeStr = "wait for orders before";
      break;
    case STOP_MODE_GRACEFULLY_POSITIONS:
      stopModeStr = "wait for positions before";
      break;
    default:
      break;
  }

  GetLog().Info(R"(Stopping with mode "%1%"...)", stopModeStr);

  OnBeforeStop();

  {
    std::vector<Strategy *> stoppedStrategies;
    for (auto &strategy : m_pimpl->m_state->m_strategies) {
      strategy->Stop(stopMode);
      stoppedStrategies.push_back(&*strategy);
    }
    for (auto strategy : stoppedStrategies) {
      strategy->WaitForStop();
    }
  }

  // Suspend events...
  m_pimpl->m_state->m_subscriptionsManager.Suspend();

  {
    auto *const dropCopy = GetDropCopy();
    if (dropCopy) {
      dropCopy->Flush();
      dropCopy->Dump();
    }
  }

  m_pimpl->m_tradingSystems.clear();
  m_pimpl->m_marketDataSources.clear();

  m_pimpl->m_state.reset();
}

std::unique_ptr<Engine::Context::AddingTransaction>
Engine::Context::StartAdding() {
  class Transaction : public AddingTransaction {
   public:
    explicit Transaction(Context &context) : m_context(context) {}
    Transaction(Transaction &&) = delete;
    Transaction(const Transaction &) = delete;
    Transaction &operator=(Transaction &&) = delete;
    Transaction &operator=(const Transaction &) = delete;
    ~Transaction() override { Transaction::Rollback(); }

    Strategy &GetStrategy(const ids::uuid &id) override {
      for (auto &strategy : m_newStrategies) {
        if (strategy->GetId() == id) {
          return *strategy;
        }
      }
      return m_context.GetStrategy(id);
    }

    void Add(const ptr::ptree &newStrategiesConf) override {
      if (m_isCommitted) {
        m_context.GetLog().Debug("Adding new entities into engine context...");
        AssertEq(0, m_newStrategies.size());
        m_context.m_pimpl->m_state->m_subscriptionsManager.Suspend();
        m_isCommitted = false;
      }

      std::vector<DllObjectPtr<Strategy>> newStrategies;
      try {
        newStrategies =
            LoadStrategies(newStrategiesConf, m_context,
                           m_context.m_pimpl->m_state->m_subscriptionsManager);
      } catch (const Exception &ex) {
        m_context.GetLog().Error(
            "Failed to add new entities into engine context: \"%1%\".", ex);
        throw Exception("Failed to add new strategies into engine context");
      }
      m_newStrategies.insert(m_newStrategies.end(),
                             std::make_move_iterator(newStrategies.begin()),
                             std::make_move_iterator(newStrategies.end()));
    }

    void Commit() override {
      if (m_isCommitted) {
        return;
      }

      auto &state = *m_context.m_pimpl->m_state;

      auto strategies = state.m_strategies;
      strategies.insert(strategies.end(), m_newStrategies.begin(),
                        m_newStrategies.end());
      state.ReportState();
      state.m_subscriptionsManager.Activate();

      m_context.ForEachMarketDataSource([&](MarketDataSource &source) {
        try {
          source.SubscribeToSecurities();
        } catch (const Lib::Exception &ex) {
          source.GetLog().Error(
              R"(Failed to make market data subscription: "%1%".)", ex);
          throw Exception("Failed to make market data subscription");
        } catch (const std::exception &ex) {
          source.GetLog().Error(
              R"(Failed to make market data subscription: "%1%".)", ex.what());
          throw;
        }
      });

      m_newStrategies.clear();
      strategies.swap(state.m_strategies);
      m_isCommitted = true;
    }

    void Rollback() noexcept override {
      if (m_isCommitted) {
        return;
      }
      m_context.GetLog().Debug(
          "New entities were not added to engine context.");
      try {
        if (!m_context.m_pimpl->m_state->m_subscriptionsManager.IsActive()) {
          m_context.m_pimpl->m_state->m_subscriptionsManager.Activate();
        }
      } catch (...) {
        AssertFailNoException();
        terminate();
      }
      m_newStrategies.clear();
      m_isCommitted = true;
    }

   private:
    Context &m_context;
    std::vector<DllObjectPtr<Strategy>> m_newStrategies;
    bool m_isCommitted = true;
  };
  return boost::make_unique<Transaction>(*this);
}

namespace {

template <typename Modules>
void UpdateModules(const ptr::ptree &conf, Modules &modules) {
  for (auto &set : modules) {
    for (auto &holder : set.second) {
      try {
        holder.module->RaiseSettingsUpdateEvent(
            IniSectionRef(conf, holder.section));
      } catch (const Exception &ex) {
        holder.module->GetLog().Error(R"(Failed to update settings: "%1%".)",
                                      ex);
      }
    }
  }
}
}  // namespace

std::unique_ptr<Engine::Context::DispatchingLock>
Engine::Context::SyncDispatching() const {
  class Lock : public DispatchingLock {
   public:
    explicit Lock(Dispatcher::UniqueSyncLock &&lock)
        : m_lock(std::move(lock)) {}

   private:
    Dispatcher::UniqueSyncLock m_lock;
  };

  return boost::make_unique<Lock>(
      m_pimpl->m_state->m_subscriptionsManager.SyncDispatching());
}

DropCopy *Engine::Context::GetDropCopy() const {
  return m_pimpl->m_state->dropCopy;
}

RiskControl &Engine::Context::GetRiskControl(const TradingMode &mode) {
  return *m_pimpl->m_riskControl[mode];
}

const RiskControl &Engine::Context::GetRiskControl(
    const TradingMode &mode) const {
  return m_pimpl->GetRiskControl(mode);
}

const ExpirationCalendar &Engine::Context::GetExpirationCalendar() const {
  if (!m_pimpl->m_expirationCalendar) {
    throw Exception("Expiration calendar is not supported");
  }
  return *m_pimpl->m_expirationCalendar;
}

bool Engine::Context::HasExpirationCalendar() const {
  return static_cast<bool>(m_pimpl->m_expirationCalendar);
}

namespace {
bool IsCurrencySupported(const std::string &symbol) {
  const auto delimiter = symbol.find('_');
  if (delimiter == std::string::npos) {
    return true;
  }
  try {
    ConvertCurrencyFromIso(symbol.substr(delimiter + 1));
  } catch (const Exception &) {
    return false;
  }
  return true;
}
}  // namespace
boost::unordered_set<std::string> Engine::Context::GetSymbolListHint() const {
  boost::unordered_set<std::string> result;
  ForEachMarketDataSource([&result](const MarketDataSource &source) {
    for (const auto &symbol : source.GetSymbolListHint()) {
      if (!IsCurrencySupported(symbol)) {
        continue;
      }
      result.emplace(symbol);
    }
  });
  return result;
}

size_t Engine::Context::GetNumberOfMarketDataSources() const {
  return m_pimpl->m_marketDataSources.size();
}

const MarketDataSource &Engine::Context::GetMarketDataSource(
    const size_t index) const {
  return const_cast<Engine::Context *>(this)->GetMarketDataSource(index);
}

MarketDataSource &Engine::Context::GetMarketDataSource(size_t index) {
  if (index >= m_pimpl->m_marketDataSources.size()) {
    throw Exception("Market Data Source index is out of range");
  }
  return *m_pimpl->m_marketDataSources[index];
}

void Engine::Context::ForEachMarketDataSource(
    const boost::function<void(const MarketDataSource &)> &callback) const {
  for (const auto &source : m_pimpl->m_marketDataSources) {
    callback(*source);
  }
}

void Engine::Context::ForEachMarketDataSource(
    const boost::function<void(MarketDataSource &)> &callback) {
  for (auto &source : m_pimpl->m_marketDataSources) {
    callback(*source);
  }
}

size_t Engine::Context::GetNumberOfTradingSystems() const {
  return m_pimpl->m_tradingSystems.size();
}

TradingSystem &Engine::Context::GetTradingSystem(const size_t index,
                                                 const TradingMode &mode) {
  if (index >= m_pimpl->m_tradingSystems.size()) {
    throw Exception("Trading System index is out of range");
  }
  if (mode != TRADING_MODE_LIVE) {
    throw TradingModeIsNotLoaded(
        "Trading System with such trading mode is not implemented");
  }
  auto &tradingSystem = m_pimpl->m_tradingSystems[index];
  if (!tradingSystem) {
    throw TradingModeIsNotLoaded(
        "Trading System with such trading mode is not loaded");
  }
  return *tradingSystem;
}

const TradingSystem &Engine::Context::GetTradingSystem(
    const size_t index, const TradingMode &mode) const {
  return const_cast<Context *>(this)->GetTradingSystem(index, mode);
}

void Engine::Context::CloseStrategiesPositions() {
  if (!m_pimpl->m_state) {
    return;
  }
  GetLog().Info("Closing positions...");
  for (auto &strategy : m_pimpl->m_state->m_strategies) {
    strategy->ClosePositions();
  }
  GetLog().Debug("Closing positions: requests sent.");
}

Strategy &Engine::Context::GetStrategy(const ids::uuid &id) {
  if (!m_pimpl->m_state) {
    throw LogicError("Is not started");
  }
  return m_pimpl->m_state->GetStrategy(id);
}

const Strategy &Engine::Context::GetStrategy(const ids::uuid &id) const {
  if (!m_pimpl->m_state) {
    throw LogicError("Is not started");
  }
  return m_pimpl->m_state->GetStrategy(id);
}

//////////////////////////////////////////////////////////////////////////
