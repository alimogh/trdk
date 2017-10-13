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
#include "Core/Observer.hpp"
#include "Core/RiskControl.hpp"
#include "Core/Service.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "Core/TradingSystem.hpp"
#include "ContextBootstrap.hpp"
#include "Ini.hpp"
#include "SubscriptionsManager.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace ids = boost::uuids;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

////////////////////////////////////////////////////////////////////////////////

namespace {

template <typename T>
size_t GetModulesCount(
    const std::map<std::string, std::vector<T>> &modulesByTag) {
  size_t result = 0;
  for (const auto &modules : modulesByTag) {
    result += modules.second.size();
  }
  return result;
}
}

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation : private boost::noncopyable {
 public:
  class State;

 public:
  Engine::Context &m_context;

  static_assert(numberOfTradingModes == 3, "List changed.");
  boost::array<std::unique_ptr<RiskControl>, numberOfTradingModes>
      m_riskControl;

  const fs::path m_fileLogsDir;

  ModuleList m_modulesDlls;

  std::unique_ptr<ExpirationCalendar> m_expirationCalendar;

  TradingSystems m_tradingSystems;
  MarketDataSources m_marketDataSources;

  bool m_isStopped;
  std::unique_ptr<State> m_state;

 public:
  explicit Implementation(Engine::Context &context, const Lib::Ini &conf)
      : m_context(context), m_isStopped(false) {
    static_assert(numberOfTradingModes == 3, "List changed.");
    for (size_t i = 0; i < m_riskControl.size(); ++i) {
      m_riskControl[i] = boost::make_unique<RiskControl>(
          m_context, conf, static_cast<TradingMode>(i));
    }

    if (conf.IsSectionExist("Expiration")) {
      const auto &expirationCalendarFilePath =
          conf.ReadFileSystemPath("Expiration", "calendar_csv");
      m_context.GetLog().Debug("Loading expiration calendar from %1%...",
                               expirationCalendarFilePath);
      auto expirationCalendar = std::make_unique<ExpirationCalendar>();
      expirationCalendar->ReloadCsv(expirationCalendarFilePath);
      const ExpirationCalendar::Stat stat = expirationCalendar->CalcStat();
      const char *const message =
          "Expiration calendar has %1% symbol(s)"
          " and %2% expiration(s).";
      stat.numberOfExpirations &&stat.numberOfSymbols
          ? m_context.GetLog().Info(message, stat.numberOfSymbols,
                                    stat.numberOfExpirations)
          : m_context.GetLog().Warn(message, stat.numberOfSymbols,
                                    stat.numberOfExpirations);
      m_expirationCalendar.swap(expirationCalendar);
    }

    BootContext(conf, m_context, m_tradingSystems, m_marketDataSources);
  }

  ~Implementation() { m_context.GetTradingLog().WaitForFlush(); }

  RiskControl &GetRiskControl(const TradingMode &mode) {
    return *m_riskControl[mode];
  }
};

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation::State : private boost::noncopyable {
 public:
  Context &context;

  SubscriptionsManager subscriptionsManager;

  Strategies strategies;
  Observers observers;
  Services services;

  DropCopy *dropCopy;

 public:
  explicit State(Context &context, DropCopy *dropCopy)
      : context(context), subscriptionsManager(context), dropCopy(dropCopy) {}

  ~State() {
    //     Assert(!subscriptionsManager.IsActive());
    // ... no new events expected, wait until old records will be flushed...
    context.GetTradingLog().WaitForFlush();
    // ... then we can destroy objects and unload DLLs...
  }

 public:
  void ReportState() const {
    {
      std::vector<std::string> markedDataSourcesStat;
      size_t commonSecuritiesCount = 0;
      context.ForEachMarketDataSource([&](const MarketDataSource &source) {
        std::ostringstream oss;
        oss << markedDataSourcesStat.size() + 1;
        if (!source.GetInstanceName().empty()) {
          oss << " (" << source.GetInstanceName() << ")";
        }
        oss << ": " << source.GetActiveSecurityCount();
        markedDataSourcesStat.push_back(oss.str());
        commonSecuritiesCount += source.GetActiveSecurityCount();
      });
      context.GetLog().Info(
          "Loaded %1% market data sources with %2% securities: %3%.",
          markedDataSourcesStat.size(), commonSecuritiesCount,
          boost::join(markedDataSourcesStat, ", "));
    }

    context.GetLog().Info("Loaded %1% observers.", observers.size());
    context.GetLog().Info("Loaded %1% strategies (%2% instances).",
                          strategies.size(), GetModulesCount(strategies));
    context.GetLog().Info("Loaded %1% services (%2% instances).",
                          services.size(), GetModulesCount(services));
  }

  Strategy &GetSrategy(const ids::uuid &id) {
    for (const auto &set : strategies) {
      for (const auto &module : set.second) {
        if (module.module->GetId() == id) {
          return *module.module;
        }
      }
    }
    throw Exception("Strategy with the given ID is not existent");
  }
};

//////////////////////////////////////////////////////////////////////////

Engine::Context::Context(
    Context::Log &log,
    Context::TradingLog &tradingLog,
    const trdk::Settings &settings,
    const Lib::Ini &conf,
    const boost::unordered_map<std::string, std::string> &params)
    : Base(log, tradingLog, settings, params),
      m_pimpl(boost::make_unique<Implementation>(*this, conf)) {}

Engine::Context::~Context() {
  if (!m_pimpl->m_isStopped) {
    Stop(STOP_MODE_IMMEDIATELY);
  }
}

void Engine::Context::Start(const Lib::Ini &conf, DropCopy *dropCopy) {
  GetLog().Debug("Starting...");

  if (m_pimpl->m_isStopped) {
    throw LogicError("Already is stopped");
  } else if (m_pimpl->m_state) {
    throw LogicError("Already is started");
  }
  Assert(m_pimpl->m_modulesDlls.empty());

  // Market data system should be connected before booting as sometimes security
  // objects can be created without market data source.
  for (auto &source : m_pimpl->m_marketDataSources) {
    try {
      source.marketDataSource->Connect(IniSectionRef(conf, source.section));
    } catch (const Interactor::ConnectError &ex) {
      boost::format message("Failed to connect to market data source: \"%1%\"");
      message % ex;
      throw Interactor::ConnectError(message.str().c_str());
    } catch (const Lib::Exception &ex) {
      GetLog().Error("Failed to make market data connection: \"%1%\".", ex);
      throw Exception("Failed to make market data connection");
    }
  }

  // It must be destroyed after state-object, as it state-object has
  // sub-objects from this DLL:

  m_pimpl->m_state.reset(new Implementation::State(*this, dropCopy));
  try {
    BootContextState(conf, *this, m_pimpl->m_state->subscriptionsManager,
                     m_pimpl->m_state->strategies, m_pimpl->m_state->observers,
                     m_pimpl->m_state->services, m_pimpl->m_modulesDlls);
  } catch (const Lib::Exception &ex) {
    GetLog().Error("Failed to init engine context: \"%1%\".", ex);
    throw Exception("Failed to init engine context");
  }

  m_pimpl->m_state->ReportState();

  for (auto &tradingSystemsByMode : m_pimpl->m_tradingSystems) {
    for (auto &tradingSystemRef : tradingSystemsByMode.holders) {
      if (!tradingSystemRef.tradingSystem) {
        AssertEq(std::string(), tradingSystemRef.section);
        continue;
      }
      Assert(!tradingSystemRef.section.empty());

      auto &tradingSystem = *tradingSystemRef.tradingSystem;
      const IniSectionRef confSection(conf, tradingSystemRef.section);

      try {
        tradingSystem.Connect(confSection);
      } catch (const Interactor::ConnectError &ex) {
        boost::format message("Failed to connect to trading system: \"%1%\"");
        message % ex;
        throw Interactor::ConnectError(message.str().c_str());
      } catch (const Lib::Exception &ex) {
        GetLog().Error("Failed to make trading system connection: \"%1%\".",
                       ex);
        throw Exception("Failed to make trading system connection");
      }
    }
  }

  ForEachMarketDataSource([&](MarketDataSource &source) -> bool {
    try {
      source.SubscribeToSecurities();
    } catch (const Interactor::ConnectError &ex) {
      boost::format message("Failed to make market data subscription: \"%1%\"");
      message % ex;
      throw Interactor::ConnectError(message.str().c_str());
    } catch (const Lib::Exception &ex) {
      GetLog().Error("Failed to make market data subscription: \"%1%\".", ex);
      throw Exception("Failed to make market data subscription");
    }
    return true;
  });

  OnStarted();

  m_pimpl->m_state->subscriptionsManager.Activate();

  RaiseStateUpdate(Context::STATE_ENGINE_STARTED);
}

void Engine::Context::Stop(const StopMode &stopMode) {
  if (m_pimpl->m_isStopped) {
    throw LogicError("Already is stopped");
  } else if (!m_pimpl->m_state) {
    return;
  }
  m_pimpl->m_isStopped = true;

  const char *stopModeStr = "unknown";
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
  }

  GetLog().Info("Stopping with mode \"%1%\"...", stopModeStr);

  OnBeforeStop();

  {
    std::vector<Strategy *> stoppedStrategies;
    for (auto &strategyies : m_pimpl->m_state->strategies) {
      for (auto &strategyHolder : strategyies.second) {
        strategyHolder.module->Stop(stopMode);
        stoppedStrategies.push_back(&*strategyHolder.module);
      }
    }
    for (Strategy *strategy : stoppedStrategies) {
      strategy->WaitForStop();
    }
  }

  // Suspend events...
  m_pimpl->m_state->subscriptionsManager.Suspend();

  {
    DropCopy *const dropCopy = GetDropCopy();
    if (dropCopy) {
      dropCopy->Flush();
      dropCopy->Dump();
    }
  }

  m_pimpl->m_marketDataSources.clear();

  m_pimpl->m_state.reset();
}

void Engine::Context::Add(const Lib::Ini &newStrategiesConf) {
  GetLog().Info("Adding new entities into engine context...");
  m_pimpl->m_state->subscriptionsManager.Suspend();

  try {
    BootNewStrategiesForContextState(
        newStrategiesConf, *this, m_pimpl->m_state->subscriptionsManager,
        m_pimpl->m_state->strategies, m_pimpl->m_state->services,
        m_pimpl->m_modulesDlls);
  } catch (const Lib::Exception &ex) {
    GetLog().Error("Failed to add new entities into engine context: \"%1%\".",
                   ex);
    throw Exception("Failed to add new strategies into engine context");
  }
  m_pimpl->m_state->ReportState();

  m_pimpl->m_state->subscriptionsManager.Activate();

  ForEachMarketDataSource([&](MarketDataSource &source) {
    try {
      source.SubscribeToSecurities();
    } catch (const Lib::Exception &ex) {
      source.GetLog().Error("Failed to make market data subscription: \"%1%\".",
                            ex);
      throw Exception("Failed to make market data subscription");
    }
  });
}

namespace {

template <typename Modules>
void UpdateModules(const Lib::Ini &conf, Modules &modules) {
  for (auto &set : modules) {
    for (auto &holder : set.second) {
      try {
        holder.module->RaiseSettingsUpdateEvent(
            IniSectionRef(conf, holder.section));
      } catch (const Lib::Exception &ex) {
        holder.module->GetLog().Error("Failed to update settings: \"%1%\".",
                                      ex);
      }
    }
  }
}
}

void Engine::Context::Update(const Lib::Ini &conf) {
  GetLog().Info("Updating setting...");

  static_assert(numberOfTradingModes == 3, "List changed.");
  for (size_t i = 0; i < numberOfTradingModes; ++i) {
    if (i == TRADING_MODE_BACKTESTING) {
      continue;
    }
    GetRiskControl(TradingMode(i)).OnSettingsUpdate(conf);
  }

  for (auto &tradingSystemByMode : m_pimpl->m_tradingSystems) {
    for (auto &tradingSystem : tradingSystemByMode.holders) {
      try {
        tradingSystem.tradingSystem->OnSettingsUpdate(
            IniSectionRef(conf, tradingSystem.section));
      } catch (const Lib::Exception &ex) {
        tradingSystem.tradingSystem->GetLog().Error(
            "Failed to update settings: \"%1%\".", ex);
      }
    }
  }

  UpdateModules(conf, m_pimpl->m_state->services);
  UpdateModules(conf, m_pimpl->m_state->strategies);
  UpdateModules(conf, m_pimpl->m_state->observers);

  GetLog().Debug("Setting update completed.");
}

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
      m_pimpl->m_state->subscriptionsManager.SyncDispatching());
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
  return m_pimpl->m_expirationCalendar ? true : false;
}

size_t Engine::Context::GetNumberOfMarketDataSources() const {
  return m_pimpl->m_marketDataSources.size();
}

const MarketDataSource &Engine::Context::GetMarketDataSource(
    size_t index) const {
  return const_cast<Engine::Context *>(this)->GetMarketDataSource(index);
}

MarketDataSource &Engine::Context::GetMarketDataSource(size_t index) {
  if (index >= m_pimpl->m_marketDataSources.size()) {
    throw Exception("Market Data Source index is out of range");
  }
  return *m_pimpl->m_marketDataSources[index].marketDataSource;
}

void Engine::Context::ForEachMarketDataSource(
    const boost::function<void(const MarketDataSource &)> &callback) const {
#ifdef BOOST_ENABLE_ASSERT_HANDLER
  size_t i = 0;
#endif
  for (const auto &source : m_pimpl->m_marketDataSources) {
    AssertEq(i++, source.marketDataSource->GetIndex());
    callback(*source.marketDataSource);
  }
}

void Engine::Context::ForEachMarketDataSource(
    const boost::function<void(MarketDataSource &)> &callback) {
#ifdef BOOST_ENABLE_ASSERT_HANDLER
  size_t i = 0;
#endif
  for (auto &source : m_pimpl->m_marketDataSources) {
    AssertEq(i++, source.marketDataSource->GetIndex());
    callback(*source.marketDataSource);
  }
}

size_t Engine::Context::GetNumberOfTradingSystems() const {
  return m_pimpl->m_tradingSystems.size();
}

TradingSystem &Engine::Context::GetTradingSystem(size_t index,
                                                 const TradingMode &mode) {
  if (index >= m_pimpl->m_tradingSystems.size()) {
    throw Exception("Trading System index is out of range");
  }
  if (mode >= m_pimpl->m_tradingSystems[index].holders.size()) {
    throw TradingModeIsNotLoaded(
        "Trading System with such trading mode is not implemented");
  }
  auto &holder = m_pimpl->m_tradingSystems[index].holders[mode];
  if (!holder.tradingSystem) {
    throw TradingModeIsNotLoaded(
        "Trading System with such trading mode is not loaded");
  }
  return *holder.tradingSystem;
}

const TradingSystem &Engine::Context::GetTradingSystem(
    size_t index, const TradingMode &mode) const {
  return const_cast<Context *>(this)->GetTradingSystem(index, mode);
}

void Engine::Context::ClosePositions() {
  if (!m_pimpl->m_state) {
    return;
  }

  GetLog().Info("Closing positions...");

  for (auto &tagetStrategies : m_pimpl->m_state->strategies) {
    for (auto &strategyHolder : tagetStrategies.second) {
      strategyHolder.module->ClosePositions();
    }
  }

  GetLog().Debug("Closing positions: requests sent.");
}

Strategy &Engine::Context::GetSrategy(const ids::uuid &id) {
  if (!m_pimpl->m_state) {
    throw LogicError("Is not started");
  }
  return m_pimpl->m_state->GetSrategy(id);
}

const Strategy &Engine::Context::GetSrategy(const ids::uuid &id) const {
  if (!m_pimpl->m_state) {
    throw LogicError("Is not started");
  }
  return m_pimpl->m_state->GetSrategy(id);
}

//////////////////////////////////////////////////////////////////////////
