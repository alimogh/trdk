//
//    Created: 2018/09/13 10:57
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WalletsConfig.hpp"
#include "Engine.hpp"

using namespace trdk;
using namespace FrontEnd;
namespace ptr = boost::property_tree;

WalletsConfig::WalletsConfig(const FrontEnd::Engine &engine)
    : m_engine(&engine) {}
WalletsConfig::WalletsConfig(const FrontEnd::Engine &engine,
                             const ptr::ptree &config)
    : m_engine(&engine), m_config(config) {}

const WalletsConfig::Symbols &WalletsConfig::Get() const {
  if (!m_symbols) {
    const_cast<WalletsConfig *>(this)->m_symbols = Load();
  }
  return *m_symbols;
}

QString WalletsConfig::GetAddress(const QString &symbol,
                                  const TradingSystem &tradingSystem) const {
  const auto &config = Get();
  const auto &symbolIt = config.find(symbol);
  if (symbolIt == config.cend()) {
    return {};
  }
  const auto &tradingSystemIt = symbolIt.value().find(&tradingSystem);
  if (tradingSystemIt == symbolIt.value().cend()) {
    return {};
  }
  return tradingSystemIt->second.wallet.address.get_value_or({});
}

void WalletsConfig::SetAddress(const QString &symbol,
                               const TradingSystem &tradingSystem,
                               const QString &address) {
  m_config.put(
      tradingSystem.GetInstanceName() + "." + symbol.toStdString() + ".address",
      address.toStdString());
  m_symbols.reset();
}
void WalletsConfig::RemoveAddress(const QString &symbol,
                                  const TradingSystem &tradingSystem) {
  auto node = m_config.get_child_optional(tradingSystem.GetInstanceName() +
                                          "." + symbol.toStdString());
  if (node) {
    node->erase("address");
    m_symbols.reset();
  }
}

void WalletsConfig::SetRecharging(const QString &symbol,
                                  const TradingSystem &tradingSystem,
                                  const Recharging &settings) {
  ptr::ptree node;
  node.add("source", settings.source->GetInstanceName());
  node.add("minDepositToRecharge", settings.minDepositToRecharge);
  node.add("minRechargingTransactionVolume",
           settings.minRechargingTransactionVolume);
  m_config.put_child(tradingSystem.GetInstanceName() + "." +
                         symbol.toStdString() + ".recharging",
                     node);
}
void WalletsConfig::RemoveRecharging(const QString &symbol,
                                     const TradingSystem &tradingSystem) {
  auto node = m_config.get_child_optional(tradingSystem.GetInstanceName() +
                                          "." + symbol.toStdString());
  if (node) {
    node->erase("recharging");
    m_symbols.reset();
  }
}

void WalletsConfig::SetSource(const QString &symbol,
                              const TradingSystem &tradingSystem,
                              const Source &settings) {
  m_config.put(tradingSystem.GetInstanceName() + "." + symbol.toStdString() +
                   ".source.minDeposit",
               settings.minDeposit);
  m_symbols.reset();
}
void WalletsConfig::RemoveSource(const QString &symbol,
                                 const TradingSystem &tradingSystem) {
  auto node = m_config.get_child_optional(tradingSystem.GetInstanceName() +
                                          "." + symbol.toStdString());
  if (node) {
    node->erase("source");
    m_symbols.reset();
  }
}

const TradingSystem *WalletsConfig::FindTradingSystem(
    const std::string &name) const {
  const auto &context = m_engine->GetContext();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    const auto &result = context.GetTradingSystem(i, TRADING_MODE_LIVE);
    if (result.GetInstanceName() == name) {
      return &result;
    }
  }
  return nullptr;
}

WalletsConfig::Symbols WalletsConfig::Load() const {
  Symbols result;
  for (const auto &tradingSystemNode : m_config) {
    const auto &tradingSystem = FindTradingSystem(tradingSystemNode.first);
    if (!tradingSystem) {
      continue;
    }
    for (const auto &symbolNode : tradingSystemNode.second) {
      auto &symbol =
          result[QString::fromStdString(symbolNode.first)][tradingSystem];

      {
        const auto &source =
            symbolNode.second.get_optional<Volume>("source.minDeposit");
        if (source) {
          symbol.source.emplace(Source{*source});
        }

        {
          const auto &address =
              symbolNode.second.get_optional<std::string>("address");
          if (address) {
            symbol.wallet.address.emplace(QString::fromStdString(*address));
          }
        }
        {
          const auto recharging =
              symbolNode.second.get_child_optional("recharging");
          if (recharging) {
            const auto &sourceTradingSystem =
                FindTradingSystem(recharging->get<std::string>("source"));
            if (!sourceTradingSystem) {
              continue;
            }
            symbol.wallet.recharging.emplace(Recharging{
                sourceTradingSystem,
                recharging->get<Volume>("minDepositToRecharge"),
                recharging->get<Volume>("minRechargingTransactionVolume")});
          }
        }
      }
    }
  }
  return result;
}

const ptr::ptree &WalletsConfig::Dump() const { return m_config; }
