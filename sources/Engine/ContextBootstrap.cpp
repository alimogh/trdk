/**************************************************************************
 *   Created: 2013/05/20 01:49:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContextBootstrap.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingSystem.hpp"
#include "SubscriptionsManager.hpp"

namespace ptr = boost::property_tree;
namespace fs = boost::filesystem;
using namespace trdk;
using namespace Lib;
using Engine::SubscriptionsManager;

namespace {

template <typename Factory>
std::pair<boost::function<Factory>, boost::shared_ptr<Dll>> LoadModuleFactory(
    const ptr::ptree &config, const char *defaultFactoryName) {
  const auto &module = Normalize(config.get<fs::path>("module"));
  std::string factoryName;
  {
    const auto &factoryNameNode = config.get_optional<std::string>("factory");
    if (factoryNameNode) {
      factoryName = *factoryNameNode;
    } else {
      factoryName = defaultFactoryName;
    }
  }
  try {
    const auto dll = boost::make_shared<Dll>(module, true);
    return {dll->GetFunction<Factory>(factoryName), dll};
  } catch (const Dll::Error &ex) {
    boost::format error(
        R"(Failed to load factory "%1%" from module %2%: "%3%")");
    error % factoryName  // 1
        % module         // 2
        % ex.what();     // 3
    throw Exception(error.str().c_str());
  }
}

}  // namespace

namespace {
std::pair<DllObjectPtr<TradingSystem>, DllObjectPtr<MarketDataSource>>
LoadTradingSystemAndMarketDataSourceModule(const ptr::ptree &config,
                                           const std::string &instanceName,
                                           const TradingMode &mode,
                                           Context &context) {
  const auto factory =
      LoadModuleFactory<TradingSystemAndMarketDataSourceFactory>(
          config, "CreateTradingSystemAndMarketDataSource");

  TradingSystemAndMarketDataSourceFactoryResult result;
  try {
    result = factory.first(mode, context, instanceName, config);
  } catch (...) {
    EventsLog::BroadcastUnhandledException(__FUNCTION__, __FILE__, __LINE__);
    throw Exception(
        "Failed to load trading system and market data source module");
  }

  if (!result.tradingSystem) {
    throw Exception(
        "Failed to load trading system and market data source module"
        " - trading system object is not existent");
  }
  if (!result.marketDataSource) {
    throw Exception(
        "Failed to load trading system and market data source module"
        " - market data source object is not existent");
  }

  return {
      DllObjectPtr<TradingSystem>{factory.second, result.tradingSystem},
      DllObjectPtr<MarketDataSource>{factory.second, result.marketDataSource}};
}
}  // namespace

std::pair<std::vector<DllObjectPtr<TradingSystem>>,
          std::vector<DllObjectPtr<MarketDataSource>>>
Engine::LoadSources(trdk::Context &context) {
  std::pair<std::vector<DllObjectPtr<TradingSystem>>,
            std::vector<DllObjectPtr<MarketDataSource>>>
      result;

  const auto &sourceList =
      context.GetSettings().GetConfig().get_child_optional("sources");
  if (!sourceList || sourceList->empty()) {
    throw Exception("No one source found in the configuration");
  }
  for (const auto &node : *sourceList) {
    const auto &instanceName = node.first;
    const auto &config = node.second;
    try {
      const auto &type = config.get<std::string>("tradingMode");
      const auto mode = type == "live"
                            ? TRADING_MODE_LIVE
                            : type == "paper" ? TRADING_MODE_PAPER
                                              : type == "backtesting"
                                                    ? TRADING_MODE_BACKTESTING
                                                    : numberOfTradingModes;
      if (mode == numberOfTradingModes) {
        throw Exception(
            (boost::format(R"(Unknown source type "%1%" is set for "%2%")") %
             type             // 1
             % instanceName)  // 2
                .str()
                .c_str());
      }
      if (config.get_child_optional("tradingSystem") ||
          config.get_child_optional("marketDataSource")) {
        throw Exception(
            "Separated objects for trading system and market data source "
            "temporary are not supported");
      }

      auto modules = LoadTradingSystemAndMarketDataSourceModule(
          config, instanceName, mode, context);
      context.GetLog().Debug("Using Trading System as Market Data Source.");
      Assert(modules.first);
      Assert(modules.second);

      result.first.emplace_back(std::move(modules.first));
      result.second.emplace_back(std::move(modules.second));
    } catch (const ptr::ptree_error &ex) {
#ifndef DEV_VER
      UseUnused(ex);
      boost::format error(
          R"(Source instance "%1%" configuration has an invalid format)");
      error % instanceName;
#else
      boost::format error(
          R"(Source instance "%1%" configuration has an invalid format: "%2%")");
      error % instanceName  // 1
          % ex.what();      // 2
#endif
      throw Exception(error.str().c_str());
    }
  }

  return result;
}

namespace {

DllObjectPtr<Strategy> LoadStrategyModule(const ptr::ptree &config,
                                          const std::string &instanceName,
                                          Context &context) {
  const auto &factory = LoadModuleFactory<std::unique_ptr<Strategy>(
      Context &, const std::string &, const ptr::ptree &)>(config,
                                                           "CreateStrategy");

  boost::shared_ptr<Strategy> result;
  try {
    result = factory.first(context, instanceName, config);
  } catch (...) {
    EventsLog::BroadcastUnhandledException(__FUNCTION__, __FILE__, __LINE__);
    throw Exception("Failed to load strategy module");
  }

  if (!result) {
    throw Exception("Failed to load strategy module - object is not existent");
  }

  return DllObjectPtr<Strategy>(factory.second, result);
}

boost::function<void(Security &)> ChooseRequrementSubscriber(
    Strategy &strategy,
    SubscriptionsManager &subscriptionsManager,
    const std::string &serviceName) {
  if (serviceName == "level1Updates") {
    return [&subscriptionsManager, &strategy](Security &security) {
      subscriptionsManager.SubscribeToLevel1Updates(security, strategy);
    };
  }
  if (serviceName == "level1Ticks") {
    return [&subscriptionsManager, &strategy](Security &security) {
      subscriptionsManager.SubscribeToLevel1Ticks(security, strategy);
    };
  }
  boost::format error(
      R"(Unknown service "%1%" in the requirements lost for instance "%2%")");
  error % serviceName                // 1
      % strategy.GetInstanceName();  // 2
  throw Exception(error.str().c_str());
}

void SubscribeToRequirements(Strategy &strategy,
                             const ptr::ptree &config,
                             Context &context,
                             SubscriptionsManager &subscriptionsManager) {
  using namespace Engine;
  for (const auto &node : config) {
    const auto &serviceName = node.first;
    const auto &symbols = node.second.get_child("symbols");
    if (symbols.empty()) {
      boost::format error(
          R"(Service "%1%" has an empty subscription list for instance "%2%")");
      error % serviceName                // 1
          % strategy.GetInstanceName();  // 2
      throw Exception(error.str().c_str());
    }

    const auto &subscribtion =
        ChooseRequrementSubscriber(strategy, subscriptionsManager, serviceName);

    for (const auto &symbolNode : symbols) {
      const auto &symbol = symbolNode.second.get_value<std::string>();
      context.ForEachMarketDataSource(
          [&strategy, &context, &subscribtion,
           &symbol](MarketDataSource &source) -> bool {
            Security *security;
            try {
              security = &source.GetSecurity(
                  Symbol{symbol, context.GetSettings().GetDefaultSecurityType(),
                         context.GetSettings().GetDefaultCurrency()});
            } catch (const SymbolIsNotSupportedException &ex) {
              context.GetLog().Debug(
                  R"(Symbol "%1%" is not supported by "%2%": "%3%".)",
                  symbol,      // 1
                  source,      // 2
                  ex.what());  // 3
              return true;
            }
            subscribtion(*security);
            strategy.RegisterSource(*security);
            return true;
          });
    }
  }
}

}  // namespace

std::vector<DllObjectPtr<Strategy>> Engine::LoadStrategies(
    const ptr::ptree &config,
    trdk::Context &context,
    SubscriptionsManager &subscriptionsManager) {
  std::vector<DllObjectPtr<Strategy>> result;
  for (const auto &node : config) {
    const auto &instanceName = node.first;
    try {
      const auto &strategyConfig = node.second;
      auto strategy = LoadStrategyModule(strategyConfig, instanceName, context);
      const auto &requerements =
          strategyConfig.get_child_optional("requirements");
      if (!requerements) {
        boost::format error(
            "Configuration for the strategy instance \"%1%\" doesn't have "
            "requirements");
        error % instanceName;
        throw Exception(error.str().c_str());
      }
      SubscribeToRequirements(*strategy, *requerements, context,
                              subscriptionsManager);

      result.emplace_back(std::move(strategy));

    } catch (const ptr::ptree_error &ex) {
#ifndef DEV_VER
      UseUnused(ex);
      boost::format error(
          "Configuration for the strategy instance \"%1%\" has an invalid "
          "format");
      error % instanceName;
#else
      boost::format error(
          "Configuration for the strategy instance \"%1%\" has an invalid "
          "format: \"%2%\"");
      error % instanceName  // 1
          % ex.what();      // 2
#endif
      throw Exception(error.str().c_str());
    }
  }

  return result;
}

std::vector<DllObjectPtr<Strategy>> Engine::LoadStrategies(
    trdk::Context &context, SubscriptionsManager &subscriptionsManager) {
  const auto &config =
      context.GetSettings().GetConfig().get_child_optional("strategies");
  if (!config) {
    return {};
  }
  return LoadStrategies(*config, context, subscriptionsManager);
}
